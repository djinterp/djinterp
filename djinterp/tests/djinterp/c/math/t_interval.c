/*******************************************************************************
* djinterp [test]                                                   t_interval.c
*
*   Conformance harness for the interval kernels (c/math/interval.h): every
* kernel of both families checked against a brute-force reference -- the
* members of an interval found by testing each integer, and the real-line
* overlap found by testing every half-integer -- over every interval with
* bounds in a small window, each bound open or closed, and steps 0 to 4; then
* the families' extremes, where a careless difference overflows.
*   The same file compiles as C++ (-x c++), at any level from C++98, and must
* print the same: the kernels are one text in both languages.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -pedantic-errors -Iinc                 \
*        tests/djinterp/c/math/t_interval.c -o t_interval
*
*
* path:      /tests/djinterp/c/math/t_interval.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <math.h>    // fabsl
#include <stdio.h>   // printf, sprintf
#include <stdlib.h>  // rand
#include <string.h>  // strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/c/math/interval.h"  // unit under test


// the window every reference interval and value is drawn from
#define D_TESTS_LOW   (-6)
#define D_TESTS_HIGH  6
#define D_TESTS_SPAN  (D_TESTS_HIGH - D_TESTS_LOW + 9)

static long g_checks   = 0;
static long g_failures = 0;

static void
d_tests_interval_expect(
    bool        _ok,
    const char* _what,
    long        _a,
    long        _b,
    int         _line
)
{
    ++g_checks;

    // report the first few failures with what was checked, and where
    if (!_ok)
    {
        ++g_failures;

        if (g_failures <= 40)
        {
            printf("  FAIL line %d: %s (%ld, %ld)\n", _line, _what, _a, _b);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, a, b)                                        \
    d_tests_interval_expect((ok), (what), (long)(a), (long)(b), __LINE__)

// d_tests_reference
//   an interval's members, found by testing each integer in its bounds.
struct d_tests_reference
{
    long   members[D_TESTS_SPAN];
    size_t count;
    long   inclusive_lower;
    long   inclusive_upper;
};

static void
d_tests_reference_build(
    struct d_tests_reference* _ref,
    long                      _lower,
    long                      _upper,
    unsigned                  _bounds,
    long                      _step
)
{
    long candidate = 0;

    _ref->count           = 0u;
    _ref->inclusive_lower = ((_bounds & D_INTERVAL_LEFT_OPEN) != 0u)
                                ? (_lower + 1) : _lower;
    _ref->inclusive_upper = ((_bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
                                ? (_upper - 1) : _upper;

    // every integer inside the bounds, on the stride from the first
    for (candidate = _ref->inclusive_lower;
         candidate <= _ref->inclusive_upper;
         ++candidate)
    {
        if ( (_step <= 0) ||
             (((candidate - _ref->inclusive_lower) % _step) == 0) )
        {
            _ref->members[_ref->count] = candidate;
            ++_ref->count;
        }
    }

    return;
}

static bool
d_tests_reference_has(
    const struct d_tests_reference* _ref,
    long                            _value
)
{
    size_t i = 0u;

    for (i = 0u; i < _ref->count; ++i)
    {
        if (_ref->members[i] == _value)
        {
            return true;
        }
    }

    return false;
}

// d_tests_real_contains
//   whether the real interval holds _twice_x / 2.
static bool
d_tests_real_contains(
    long     _lower,
    long     _upper,
    unsigned _bounds,
    long     _twice_x
)
{
    const bool left  = ((_bounds & D_INTERVAL_LEFT_OPEN) != 0u)
                           ? (_twice_x > (2 * _lower))
                           : (_twice_x >= (2 * _lower));
    const bool right = ((_bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
                           ? (_twice_x < (2 * _upper))
                           : (_twice_x <= (2 * _upper));

    return (left && right);
}

static bool
d_tests_close(
    long double _got,
    long double _want
)
{
    const long double scale = (fabsl(_want) > 1.0L) ? fabsl(_want) : 1.0L;

    return (fabsl(_got - _want) <= (1e-15L * scale));
}

// d_tests_interval_one
//   every single-interval kernel of both families against the reference.
static void
d_tests_interval_one(
    long     _lower,
    long     _upper,
    unsigned _bounds,
    long     _step
)
{
    struct d_tests_reference     ref;
    const struct d_interval_imax iv  = d_interval_imax_make(_lower,
                                                            _upper,
                                                            _bounds,
                                                            _step);
    const long                   off = 64;   // shifts the window to >= 0
    const struct d_interval_umax uv  = d_interval_umax_make(
                                           (d_math_umax)(_lower + off),
                                           (d_math_umax)(_upper + off),
                                           _bounds,
                                           (d_math_umax)_step);
    long   value = 0;
    size_t i     = 0u;

    d_tests_reference_build(&ref, _lower, _upper, _bounds, _step);

    D_TESTS_EXPECT(d_interval_imax_is_valid(iv), "imax valid", _lower, _upper);
    D_TESTS_EXPECT(d_interval_umax_is_valid(uv), "umax valid", _lower, _upper);
    D_TESTS_EXPECT(d_interval_imax_count(iv) == ref.count,
                   "imax count", _lower, _upper);
    D_TESTS_EXPECT(d_interval_umax_count(uv) == ref.count,
                   "umax count", _lower, _upper);
    D_TESTS_EXPECT(d_interval_imax_is_empty(iv) == (ref.count == 0u),
                   "imax empty", _lower, _upper);
    D_TESTS_EXPECT(d_interval_umax_is_empty(uv) == (ref.count == 0u),
                   "umax empty", _lower, _upper);
    D_TESTS_EXPECT(d_interval_imax_inclusive_lower(iv) == ref.inclusive_lower,
                   "imax inclusive_lower", _lower, _upper);
    D_TESTS_EXPECT(d_interval_imax_inclusive_upper(iv) == ref.inclusive_upper,
                   "imax inclusive_upper", _lower, _upper);

    // first, last and every member by index, where there are members
    if (ref.count > 0u)
    {
        D_TESTS_EXPECT(d_interval_imax_first(iv) == ref.members[0],
                       "imax first", _lower, _upper);
        D_TESTS_EXPECT(d_interval_imax_last(iv) == ref.members[ref.count - 1u],
                       "imax last", _lower, _upper);
        D_TESTS_EXPECT( (long)d_interval_umax_first(uv) - off ==
                        ref.members[0],
                        "umax first", _lower, _upper);
        D_TESTS_EXPECT( (long)d_interval_umax_last(uv) - off ==
                        ref.members[ref.count - 1u],
                        "umax last", _lower, _upper);

        for (i = 0u; i < ref.count; ++i)
        {
            D_TESTS_EXPECT(d_interval_imax_at(iv, i) == ref.members[i],
                           "imax at", _lower, (long)i);
            D_TESTS_EXPECT( (long)d_interval_umax_at(uv, i) - off ==
                            ref.members[i],
                            "umax at", _lower, (long)i);
        }
    }

    // an empty interval still answers, as signed arithmetic on its inclusive
    // bounds would: the banner's promise, and the two families must agree
    if (ref.count == 0u)
    {
        const long il   = ref.inclusive_lower;
        const long iu   = ref.inclusive_upper;
        const long last = (_step > 0) ? (il + (((iu - il) / _step) * _step))
                                      : iu;

        D_TESTS_EXPECT(d_interval_imax_first(iv) == il,
                       "imax empty first", _lower, _upper);
        D_TESTS_EXPECT(d_interval_imax_last(iv) == last,
                       "imax empty last", _lower, _upper);
        D_TESTS_EXPECT((long)d_interval_umax_last(uv) - off == last,
                       "umax empty last", _lower, _upper);

        for (value = D_TESTS_LOW - 4; value <= D_TESTS_HIGH + 4; ++value)
        {
            const long want = (value < il) ? il : last;

            D_TESTS_EXPECT(d_interval_imax_clamp(iv, value) == want,
                           "imax empty clamp", _lower, value);
            D_TESTS_EXPECT( (long)d_interval_umax_clamp(
                                uv,
                                (d_math_umax)(value + off)) - off == want,
                            "umax empty clamp", _lower, value);
        }
    }

    // every value in a window around the bounds
    for (value = D_TESTS_LOW - 4; value <= D_TESTS_HIGH + 4; ++value)
    {
        const d_math_umax uvalue    = (d_math_umax)(value + off);
        const bool        member    = d_tests_reference_has(&ref, value);
        const bool        in_bounds =
            d_tests_real_contains(_lower, _upper, _bounds, 2 * value);
        size_t            index     = ref.count;

        D_TESTS_EXPECT(d_interval_imax_contains(iv, value) == member,
                       "imax contains", _lower, value);
        D_TESTS_EXPECT(d_interval_umax_contains(uv, uvalue) == member,
                       "umax contains", _lower, value);
        D_TESTS_EXPECT(d_interval_imax_contains_in_range(iv, value) ==
                       in_bounds,
                       "imax contains_in_range", _lower, value);
        D_TESTS_EXPECT(d_interval_umax_contains_in_range(uv, uvalue) ==
                       in_bounds,
                       "umax contains_in_range", _lower, value);

        // the index of a member, or the count for a non-member
        for (i = 0u; i < ref.count; ++i)
        {
            if (ref.members[i] == value)
            {
                index = i;
            }
        }

        D_TESTS_EXPECT(d_interval_imax_index_of(iv, value) == index,
                       "imax index_of", _lower, value);
        D_TESTS_EXPECT(d_interval_umax_index_of(uv, uvalue) == index,
                       "umax index_of", _lower, value);

        // clamping is checked where there is a member to clamp to
        if (ref.count > 0u)
        {
            long clamped = value;
            long nearest = value;

            if (value < ref.inclusive_lower)
            {
                clamped = ref.members[0];
            }
            else if (value > ref.inclusive_upper)
            {
                clamped = ref.members[ref.count - 1u];
            }
            else if (_step > 0)
            {
                for (i = 0u; i < ref.count; ++i)
                {
                    if (ref.members[i] <= value)
                    {
                        clamped = ref.members[i];
                    }
                }
            }

            nearest = clamped;

            // the nearer member of a discrete interval, a tie going down
            if (_step > 0)
            {
                long best = ref.members[0];

                for (i = 0u; i < ref.count; ++i)
                {
                    const long d_new  = (ref.members[i] > value)
                                            ? (ref.members[i] - value)
                                            : (value - ref.members[i]);
                    const long d_best = (best > value) ? (best - value)
                                                       : (value - best);

                    if (d_new < d_best)
                    {
                        best = ref.members[i];
                    }
                }

                nearest = best;
            }

            D_TESTS_EXPECT(d_interval_imax_clamp(iv, value) == clamped,
                           "imax clamp", _lower, value);
            D_TESTS_EXPECT( (long)d_interval_umax_clamp(uv, uvalue) - off ==
                            clamped,
                            "umax clamp", _lower, value);
            D_TESTS_EXPECT(d_interval_imax_clamp_nearest(iv, value) == nearest,
                           "imax clamp_nearest", _lower, value);
            D_TESTS_EXPECT( (long)d_interval_umax_clamp_nearest(uv, uvalue) -
                            off == nearest,
                            "umax clamp_nearest", _lower, value);
        }

        // normalization against the formulas, in long double
        {
            const long        il   = ref.inclusive_lower;
            const long        iu   = ref.inclusive_upper;
            const long double full =
                (_upper == _lower)
                    ? 0.0L
                    : ((long double)(value - _lower) /
                       (long double)(_upper - _lower));
            const long double eff  =
                (iu == il) ? 0.0L
                           : ((long double)(value - il) /
                              (long double)(iu - il));
            long double       disc = eff;

            if (_step > 0)
            {
                disc = (ref.count <= 1u)
                           ? 0.0L
                           : ((long double)((value - il) / _step) /
                              (long double)(ref.count - 1u));
            }

            D_TESTS_EXPECT(d_tests_close(d_interval_imax_normalize(iv, value),
                                         full),
                           "imax normalize", _lower, value);
            D_TESTS_EXPECT(d_tests_close(d_interval_umax_normalize(uv, uvalue),
                                         full),
                           "umax normalize", _lower, value);
            D_TESTS_EXPECT(d_tests_close(
                               d_interval_imax_normalize_effective(iv, value),
                               eff),
                           "imax normalize_effective", _lower, value);
            D_TESTS_EXPECT(d_tests_close(
                               d_interval_umax_normalize_effective(uv, uvalue),
                               eff),
                           "umax normalize_effective", _lower, value);
            D_TESTS_EXPECT(d_tests_close(
                               d_interval_imax_normalize_discrete(iv, value),
                               disc),
                           "imax normalize_discrete", _lower, value);
            D_TESTS_EXPECT(d_tests_close(
                               d_interval_umax_normalize_discrete(uv, uvalue),
                               disc),
                           "umax normalize_discrete", _lower, value);
        }
    }

    // the text form, against printf's
    {
        char       want[96];
        char       got[96];
        const char left  = ((_bounds & D_INTERVAL_LEFT_OPEN) != 0u)  ? '('
                                                                     : '[';
        const char right = ((_bounds & D_INTERVAL_RIGHT_OPEN) != 0u) ? ')'
                                                                     : ']';
        size_t     len   = 0u;

        if (_step > 0)
        {
            sprintf(want, "%c%ld:%ld:%ld%c", left, _lower, _step, _upper,
                    right);
        }
        else
        {
            sprintf(want, "%c%ld, %ld%c", left, _lower, _upper, right);
        }

        len = d_interval_imax_format(got, sizeof(got), iv);
        D_TESTS_EXPECT((strcmp(got, want) == 0) && (len == strlen(want)),
                       "imax format", _lower, _upper);
        D_TESTS_EXPECT(d_interval_imax_format(NULL, 0u, iv) == strlen(want),
                       "imax format measures", _lower, _upper);
    }

    return;
}

// d_tests_interval_pair
//   both overlap kernels of both families, for one pair, against the
// reference: integer ranges for overlaps, the real line for the other.
static void
d_tests_interval_pair(
    long     _al,
    long     _ah,
    unsigned _ab,
    long     _bl,
    long     _bh,
    unsigned _bb
)
{
    struct d_tests_reference ra;
    struct d_tests_reference rb;
    bool                     real  = false;
    long                     twice = 0;

    d_tests_reference_build(&ra, _al, _ah, _ab, 0);
    d_tests_reference_build(&rb, _bl, _bh, _bb, 0);

    const bool ranges = ( (ra.count > 0u) &&
                          (rb.count > 0u) &&
                          !( (ra.inclusive_upper < rb.inclusive_lower) ||
                             (ra.inclusive_lower > rb.inclusive_upper) ) );

    // a common point, if there is one, is an integer or a half-integer
    for (twice = -8; twice <= 8; ++twice)
    {
        if ( (d_tests_real_contains(_al, _ah, _ab, twice)) &&
             (d_tests_real_contains(_bl, _bh, _bb, twice)) )
        {
            real = true;
        }
    }

    const struct d_interval_imax ia = d_interval_imax_make(_al, _ah, _ab, 0);
    const struct d_interval_imax ib = d_interval_imax_make(_bl, _bh, _bb, 0);
    const struct d_interval_umax ua =
        d_interval_umax_make((d_math_umax)(_al + 8),
                             (d_math_umax)(_ah + 8),
                             _ab,
                             0u);
    const struct d_interval_umax ub =
        d_interval_umax_make((d_math_umax)(_bl + 8),
                             (d_math_umax)(_bh + 8),
                             _bb,
                             0u);
    const long tag_a = (_al * 100) + _ah;
    const long tag_b = (_bl * 100) + _bh;

    D_TESTS_EXPECT(d_interval_imax_overlaps(ia, ib) == ranges,
                   "imax overlaps", tag_a, tag_b);
    D_TESTS_EXPECT(d_interval_umax_overlaps(ua, ub) == ranges,
                   "umax overlaps", tag_a, tag_b);
    D_TESTS_EXPECT(d_interval_imax_overlaps_continuous(ia, ib) == real,
                   "imax overlaps_continuous", tag_a, tag_b);
    D_TESTS_EXPECT(d_interval_umax_overlaps_continuous(ua, ub) == real,
                   "umax overlaps_continuous", tag_a, tag_b);

    return;
}

// d_tests_interval_pairs
//   both overlap kernels, for every pair of continuous intervals with bounds
// in a smaller window.
static void
d_tests_interval_pairs(void)
{
    long     lows[128];
    long     highs[128];
    unsigned kinds[128];
    size_t   count = 0u;
    size_t   a     = 0u;
    size_t   b     = 0u;
    long     lower = 0;

    // every interval with bounds in [-3, 3], each bound open or closed
    for (lower = -3; lower <= 3; ++lower)
    {
        long upper = 0;

        for (upper = lower; upper <= 3; ++upper)
        {
            unsigned kind = 0u;

            for (kind = 0u; kind < 4u; ++kind)
            {
                lows[count]  = lower;
                highs[count] = upper;
                kinds[count] = kind;
                ++count;
            }
        }
    }

    for (a = 0u; a < count; ++a)
    {
        for (b = 0u; b < count; ++b)
        {
            d_tests_interval_pair(lows[a], highs[a], kinds[a],
                                  lows[b], highs[b], kinds[b]);
        }
    }

    return;
}

// d_tests_interval_intersects
//   intersects, both families, for every pair of intervals with bounds in
// [-4, 4], each bound open or closed, steps 0 to 3, against the reference's
// member sets; then lattices far apart and wide, where a careless product
// would overflow.
static void
d_tests_interval_intersects(void)
{
    struct d_tests_reference refs[1024];
    long                     lows[1024];
    long                     highs[1024];
    unsigned                 kinds[1024];
    long                     steps[1024];
    size_t                   count = 0u;
    size_t                   a     = 0u;
    long                     lower = 0;

    // every interval of the window, with its members
    for (lower = -4; lower <= 4; ++lower)
    {
        long upper = 0;

        for (upper = lower; upper <= 4; ++upper)
        {
            unsigned kind = 0u;

            for (kind = 0u; kind < 4u; ++kind)
            {
                long step = 0;

                for (step = 0; step <= 3; ++step)
                {
                    d_tests_reference_build(&refs[count], lower, upper, kind,
                                            step);
                    lows[count]  = lower;
                    highs[count] = upper;
                    kinds[count] = kind;
                    steps[count] = step;
                    ++count;
                }
            }
        }
    }

    for (a = 0u; a < count; ++a)
    {
        const struct d_interval_imax ia = d_interval_imax_make(lows[a],
                                                               highs[a],
                                                               kinds[a],
                                                               steps[a]);
        const struct d_interval_umax ua =
            d_interval_umax_make((d_math_umax)(lows[a] + 8),
                                 (d_math_umax)(highs[a] + 8),
                                 kinds[a],
                                 (d_math_umax)steps[a]);
        size_t b = 0u;

        for (b = 0u; b < count; ++b)
        {
            const struct d_interval_imax ib = d_interval_imax_make(lows[b],
                                                                   highs[b],
                                                                   kinds[b],
                                                                   steps[b]);
            const struct d_interval_umax ub =
                d_interval_umax_make((d_math_umax)(lows[b] + 8),
                                     (d_math_umax)(highs[b] + 8),
                                     kinds[b],
                                     (d_math_umax)steps[b]);
            bool   shared = false;
            size_t i      = 0u;

            // a member of one that the other has
            for (i = 0u; i < refs[a].count; ++i)
            {
                if (d_tests_reference_has(&refs[b], refs[a].members[i]))
                {
                    shared = true;
                }
            }

            D_TESTS_EXPECT(d_interval_imax_intersects(ia, ib) == shared,
                           "imax intersects", (long)a, (long)b);
            D_TESTS_EXPECT(d_interval_umax_intersects(ua, ub) == shared,
                           "umax intersects", (long)a, (long)b);
        }
    }

    // the Chinese remainder theorem's cases: x = 0 mod 6 and x = 4 mod 10
    // meet at 24; x = 0 mod 6 and x = 1 mod 10 never (parity)
    D_TESTS_EXPECT(d_interval_imax_intersects(
                       d_interval_imax_make(0, 1000, D_INTERVAL_CLOSED, 6),
                       d_interval_imax_make(4, 1000, D_INTERVAL_CLOSED, 10)),
                   "imax lattices meeting at 24", 0, 0);
    D_TESTS_EXPECT(!d_interval_imax_intersects(
                       d_interval_imax_make(0, 23, D_INTERVAL_CLOSED, 6),
                       d_interval_imax_make(4, 1000, D_INTERVAL_CLOSED, 10)),
                   "imax lattices meeting only past a range", 0, 0);
    D_TESTS_EXPECT(!d_interval_imax_intersects(
                       d_interval_imax_make(0, 1000, D_INTERVAL_CLOSED, 6),
                       d_interval_imax_make(1, 1000, D_INTERVAL_CLOSED, 10)),
                   "imax lattices of different parity", 0, 0);

    // across the whole signed family, with steps whose product overflows
    {
        const d_math_imax lo  = D_MATH_IMAX_MIN;
        const d_math_imax hi  = D_MATH_IMAX_MAX;
        const d_math_imax big = (d_math_imax)(D_MATH_IMAX_MAX / 3);

        D_TESTS_EXPECT(d_interval_imax_intersects(
                           d_interval_imax_make(lo, hi, D_INTERVAL_CLOSED, big),
                           d_interval_imax_make(lo, hi, D_INTERVAL_CLOSED,
                                                big - 1)),
                       "imax wide lattices share their first member", 0, 0);
        D_TESTS_EXPECT(!d_interval_imax_intersects(
                           d_interval_imax_make(lo, hi, D_INTERVAL_CLOSED, big),
                           d_interval_imax_make(lo + 1, hi, D_INTERVAL_CLOSED,
                                                big)),
                       "imax wide lattices one apart never meet", 0, 0);
        D_TESTS_EXPECT(d_interval_umax_intersects(
                           d_interval_umax_make(0u, D_MATH_UMAX_MAX,
                                                D_INTERVAL_CLOSED, 2u),
                           d_interval_umax_make(D_MATH_UMAX_MAX - 1u,
                                                D_MATH_UMAX_MAX,
                                                D_INTERVAL_CLOSED, 0u)),
                       "umax even lattice reaches the family's top", 0, 0);
    }

    return;
}

// d_tests_interval_number_theory
//   the lattice helpers on their own -- mulmod against exact products, the
// inverse against its definition -- and intersects over random lattices
// with steps up to 60 against brute force: periods the window above never
// reaches.
static void
d_tests_interval_number_theory(void)
{
    unsigned long state = 12345u;
    long          i     = 0;

    for (i = 0; i < 200000; ++i)
    {
        d_math_umax a = 0u;
        d_math_umax b = 0u;
        d_math_umax m = 0u;

        // three values below 2^31, so a * b is exact in 64 bits
        state = (state * 1103515245u) + 12345u;
        a     = (d_math_umax)((state >> 1) & 0x7FFFFFFFu);
        state = (state * 1103515245u) + 12345u;
        b     = (d_math_umax)((state >> 1) & 0x7FFFFFFFu);
        state = (state * 1103515245u) + 12345u;
        m     = (d_math_umax)(((state >> 1) & 0x7FFFFFFFu) | 1u);

        if (sizeof(d_math_umax) >= 8u)
        {
            D_TESTS_EXPECT(d_internal_interval_mulmod(a, b, m) == (a * b) % m,
                           "mulmod", (long)a, (long)m);
        }

        // where a and m are coprime, a * inverse = 1 modulo m
        if (d_internal_interval_gcd(a, m) == 1u)
        {
            const d_math_umax inv = d_internal_interval_inverse(a, m);

            D_TESTS_EXPECT( (m == 1u) ||
                            (d_internal_interval_mulmod(a, inv, m) == 1u),
                            "inverse", (long)a, (long)m);
        }
    }

    // every small modulus exhaustively: the reduction's boundaries
    {
        d_math_umax m = 1u;

        for (m = 1u; m <= 64u; ++m)
        {
            d_math_umax a = 0u;

            for (a = 0u; a < 3u * m; ++a)
            {
                d_math_umax b = 0u;

                for (b = 0u; b < 3u * m; ++b)
                {
                    D_TESTS_EXPECT(d_internal_interval_mulmod(a, b, m) ==
                                   (a * b) % m,
                                   "mulmod, small modulus", (long)a, (long)m);
                }

                // the inverse, wherever it exists
                if (d_internal_interval_gcd(a % m, m) == 1u)
                {
                    D_TESTS_EXPECT( (m == 1u) ||
                                    ( (d_internal_interval_inverse(a, m) *
                                       (a % m)) % m == 1u ),
                                    "inverse, small modulus", (long)a,
                                    (long)m);
                }
            }
        }
    }

    // full-width products, where the compiler has a wider type to check by
#if defined(__SIZEOF_INT128__)
    for (i = 0; i < 200000; ++i)
    {
        __extension__ typedef unsigned __int128 d_tests_u128;

        const d_math_umax  a     = ((d_math_umax)rand() << 33) ^
                                   (d_math_umax)rand();
        const d_math_umax  b     = ((d_math_umax)rand() << 35) ^
                                   (d_math_umax)rand();
        const d_math_umax  m     = (((d_math_umax)rand() << 34) ^
                                    (d_math_umax)rand()) | 1u;
        const d_tests_u128 exact = ((d_tests_u128)a * (d_tests_u128)b) %
                                   (d_tests_u128)m;

        D_TESTS_EXPECT(d_internal_interval_mulmod(a, b, m) ==
                       (d_math_umax)exact,
                       "mulmod at full width", (long)i, 0);
    }
#endif  // __SIZEOF_INT128__

    // random lattices against brute force
    for (i = 0; i < 20000; ++i)
    {
        long        a0 = 0, a_step = 0, a_n = 0, b0 = 0, b_step = 0, b_n = 0;
        long        x  = 0;
        bool        shared = false;

        state  = (state * 1103515245u) + 12345u;
        a0     = (long)((state >> 8) % 400u) - 200;
        state  = (state * 1103515245u) + 12345u;
        a_step = (long)((state >> 8) % 60u) + 1;
        state  = (state * 1103515245u) + 12345u;
        a_n    = (long)((state >> 8) % 40u);
        state  = (state * 1103515245u) + 12345u;
        b0     = (long)((state >> 8) % 400u) - 200;
        state  = (state * 1103515245u) + 12345u;
        b_step = (long)((state >> 8) % 60u) + 1;
        state  = (state * 1103515245u) + 12345u;
        b_n    = (long)((state >> 8) % 40u);

        // a member of the first that the second has
        for (x = a0; x <= a0 + (a_n * a_step); x += a_step)
        {
            if ( (x >= b0) &&
                 (x <= b0 + (b_n * b_step)) &&
                 (((x - b0) % b_step) == 0) )
            {
                shared = true;
            }
        }

        D_TESTS_EXPECT(d_interval_imax_intersects(
                           d_interval_imax_make(a0, a0 + (a_n * a_step),
                                                D_INTERVAL_CLOSED, a_step),
                           d_interval_imax_make(b0, b0 + (b_n * b_step),
                                                D_INTERVAL_CLOSED, b_step)) ==
                       shared,
                       "imax intersects, random lattices", a_step, b_step);
    }

    return;
}

// d_tests_interval_extremes
//   the families' ends, where a difference taken in the signed type would
// overflow and a count would not fit.
static void
d_tests_interval_extremes(void)
{
    const d_math_imax lo = D_MATH_IMAX_MIN;
    const d_math_imax hi = D_MATH_IMAX_MAX;
    const struct d_interval_imax all =
        d_interval_imax_make(lo, hi, D_INTERVAL_CLOSED, 0);
    const struct d_interval_imax all_open =
        d_interval_imax_make(lo, hi, D_INTERVAL_OPEN, 0);
    const struct d_interval_imax wide_step =
        d_interval_imax_make(lo, hi, D_INTERVAL_CLOSED, hi);
    const struct d_interval_umax uall =
        d_interval_umax_make(0u, D_MATH_UMAX_MAX, D_INTERVAL_CLOSED, 0u);
    const struct d_interval_umax utop =
        d_interval_umax_make(D_MATH_UMAX_MAX - 2u, D_MATH_UMAX_MAX,
                             D_INTERVAL_RIGHT_OPEN, 0u);
    char text[96];

    // the whole family has one member more than d_math_umax counts
    D_TESTS_EXPECT(d_interval_imax_count(all) == D_MATH_UMAX_MAX,
                   "imax full count saturates", 0, 0);
    D_TESTS_EXPECT(d_interval_umax_count(uall) == D_MATH_UMAX_MAX,
                   "umax full count saturates", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_count(all_open) == D_MATH_UMAX_MAX - 1u,
                   "imax open count", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_count(wide_step) == 3u,
                   "imax wide step count", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_at(wide_step, 2u) == hi - 1,
                   "imax wide step at", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_last(wide_step) == hi - 1,
                   "imax wide step last", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_contains(all, lo) &&
                   d_interval_imax_contains(all, hi),
                   "imax contains its ends", 0, 0);
    D_TESTS_EXPECT(!d_interval_imax_contains(all_open, lo) &&
                   !d_interval_imax_contains(all_open, hi),
                   "imax open excludes its ends", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_clamp(all_open, lo) == lo + 1,
                   "imax open clamp low", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_clamp(all_open, hi) == hi - 1,
                   "imax open clamp high", 0, 0);
    D_TESTS_EXPECT(d_interval_imax_index_of(all, hi) == D_MATH_UMAX_MAX,
                   "imax index_of the top", 0, 0);
    D_TESTS_EXPECT(d_tests_close(d_interval_imax_normalize(all, hi), 1.0L) &&
                   d_tests_close(d_interval_imax_normalize(all, lo), 0.0L),
                   "imax normalize ends", 0, 0);
    D_TESTS_EXPECT(d_interval_umax_count(utop) == 2u,
                   "umax top count", 0, 0);
    D_TESTS_EXPECT(d_interval_umax_last(utop) == D_MATH_UMAX_MAX - 1u,
                   "umax top last", 0, 0);

    // the family's minimum is written with its sign
    (void)d_interval_imax_format(text, sizeof(text), all);
    D_TESTS_EXPECT( strcmp(text,
                           (sizeof(d_math_imax) == 8u)
                               ? "[-9223372036854775808, 9223372036854775807]"
                               : "[-2147483648, 2147483647]") == 0,
                    "imax format extremes", (long)sizeof(d_math_imax), 0);

    // a short buffer is filled and terminated, and the length still reported
    {
        char         small[6];
        const size_t full = d_interval_imax_format(small, sizeof(small), all);

        D_TESTS_EXPECT( (full == strlen(text)) &&
                        (strlen(small) == sizeof(small) - 1u) &&
                        (strncmp(small, text, sizeof(small) - 1u) == 0),
                        "imax format truncates", (long)full, 0);
    }

    return;
}

int
main(void)
{
    long lower = 0;

    // every interval with bounds in the window, every bound kind and step
    for (lower = D_TESTS_LOW; lower <= D_TESTS_HIGH; ++lower)
    {
        long upper = 0;

        for (upper = lower; upper <= D_TESTS_HIGH; ++upper)
        {
            unsigned bounds = 0u;

            for (bounds = 0u; bounds < 4u; ++bounds)
            {
                long step = 0;

                for (step = 0; step <= 4; ++step)
                {
                    d_tests_interval_one(lower, upper, bounds, step);
                }
            }
        }
    }

    d_tests_interval_pairs();
    d_tests_interval_intersects();
    d_tests_interval_number_theory();
    d_tests_interval_extremes();

    printf("t_interval: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
