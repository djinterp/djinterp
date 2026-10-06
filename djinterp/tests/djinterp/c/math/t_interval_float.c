/*******************************************************************************
* djinterp [test]                                             t_interval_float.c
*
*   Conformance harness for the floating-point interval families
* (c/math/interval_float.h). The float family against brute force: every
* interval whose bounds are two of 13 consecutive floats -- near 1, across
* zero, among the subnormals, at the top of the range -- each bound open or
* closed, continuous or with a stride of 1 to 3 ulps or a decimal one; the
* members found by stepping with nextafterf, every kernel checked at every
* float of the window and the real-line overlap at every midpoint. Then the
* double and long double families at the cases that differ: a decimal
* stride, a count across a binade, an open bound's neighbour, the text.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -pedantic-errors -Iinc                 \
*        tests/djinterp/c/math/t_interval_float.c -o t_interval_float -lm
*
*
* path:      /tests/djinterp/c/math/t_interval_float.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <float.h>   // FLT_MIN, FLT_MAX, DBL_EPSILON
#include <math.h>    // nextafterf, HUGE_VAL
#include <stdio.h>   // printf, sscanf
#include <string.h>  // strcmp
// djinterp
#include "../../../../inc/djinterp/c/math/interval_float.h"  // unit under test


#define D_TESTS_WINDOW 13

static long g_checks   = 0;
static long g_failures = 0;

static void
d_tests_expect(
    bool        _ok,
    const char* _what,
    double      _a,
    double      _b,
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
            printf("  FAIL line %d: %s (%.9g, %.9g)\n", _line, _what, _a, _b);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what, a, b)                                        \
    d_tests_expect((ok), (what), (double)(a), (double)(b), __LINE__)

// d_tests_window
//   the window: 13 consecutive floats from _start, and the members of an
// interval among them found by brute force
static float g_window[D_TESTS_WINDOW + 8];

static void
d_tests_window_fill(float _start)
{
    float  x = nextafterf(nextafterf(nextafterf(nextafterf(_start, -HUGE_VAL),
                                                -HUGE_VAL),
                                     -HUGE_VAL),
                          -HUGE_VAL);
    size_t i = 0u;

    // four values below the window, the window, four above
    for (i = 0u; i < D_TESTS_WINDOW + 8u; ++i)
    {
        g_window[i] = x;
        x           = nextafterf(x, HUGE_VAL);
    }

    return;
}

// (a stride of the smaller spacing below a binade edge can fit twice as
// many members as the window has floats, so room for 64)
struct d_tests_members
{
    float  values[64];
    size_t count;
};

static void
d_tests_members_of(
    struct d_tests_members* _out,
    struct d_interval_f     _iv
)
{
    size_t i = 0u;

    _out->count = 0u;

    // continuous: every float in the bounds; discrete: first + k * step
    if (_iv.step > 0.0f)
    {
        float       first = 0.0f;
        bool        found = false;
        d_math_umax k     = 0u;

        for (i = 0u; i < D_TESTS_WINDOW + 8u; ++i)
        {
            if ( (!found) &&
                 d_interval_f_contains_in_range(_iv, g_window[i]) )
            {
                first = g_window[i];
                found = true;
            }
        }

        // (an infinite step leaves one member, the first)
        for (k = 0u; found && (k < 64u); ++k)
        {
            const float offset = (k == 0u) ? 0.0f : ((float)k * _iv.step);
            const float member = (k == 0u) ? first : (first + offset);

            if ( (k > 0u) &&
                 (_iv.step > FLT_MAX) )
            {
                break;
            }

            if (!d_interval_f_contains_in_range(_iv, member))
            {
                break;
            }

            // members past the largest float are one, +inf
            if ( (_out->count > 0u) &&
                 (_out->values[_out->count - 1u] == member) )
            {
                continue;
            }

            _out->values[_out->count] = member;
            ++_out->count;
        }

        return;
    }

    // (past the largest float the window repeats +inf: each value once)
    for (i = 0u; i < D_TESTS_WINDOW + 8u; ++i)
    {
        if ( d_interval_f_contains_in_range(_iv, g_window[i]) &&
             ( (i == 0u) ||
               (g_window[i] != g_window[i - 1u]) ) )
        {
            _out->values[_out->count] = g_window[i];
            ++_out->count;
        }
    }

    return;
}

static bool
d_tests_members_has(
    const struct d_tests_members* _m,
    float                         _x
)
{
    size_t i = 0u;

    for (i = 0u; i < _m->count; ++i)
    {
        if (_m->values[i] == _x)
        {
            return true;
        }
    }

    return false;
}

// d_tests_one
//   one interval: every single-interval kernel against brute force.
static void
d_tests_one(struct d_interval_f _iv)
{
    struct d_tests_members m;
    size_t                 i = 0u;

    d_tests_members_of(&m, _iv);

    D_TESTS_EXPECT(d_interval_f_count(_iv) == m.count, "count",
                   _iv.lower, _iv.upper);
    D_TESTS_EXPECT(d_interval_f_is_empty(_iv) == (m.count == 0u), "is_empty",
                   _iv.lower, _iv.upper);

    // first, last, every member by index and back
    if (m.count > 0u)
    {
        D_TESTS_EXPECT(d_interval_f_first(_iv) == m.values[0], "first",
                       _iv.lower, _iv.upper);
        D_TESTS_EXPECT(d_interval_f_last(_iv) == m.values[m.count - 1u],
                       "last", _iv.lower, _iv.upper);

        for (i = 0u; i < m.count; ++i)
        {
            D_TESTS_EXPECT(d_interval_f_at(_iv, i) == m.values[i], "at",
                           _iv.lower, (double)i);
            D_TESTS_EXPECT(d_interval_f_index_of(_iv, m.values[i]) == i,
                           "index_of", _iv.lower, (double)i);
        }
    }

    // every float of the window
    for (i = 0u; i < D_TESTS_WINDOW + 8u; ++i)
    {
        const float x      = g_window[i];
        const bool  member = d_tests_members_has(&m, x);

        D_TESTS_EXPECT(d_interval_f_contains(_iv, x) == member, "contains",
                       _iv.lower, x);

        if (!member)
        {
            D_TESTS_EXPECT(d_interval_f_index_of(_iv, x) == m.count,
                           "index_of a non-member", _iv.lower, x);
        }

        // clamping: to the nearest member toward the interval
        if (m.count > 0u)
        {
            float  down    = m.values[0];
            float  nearest = m.values[0];
            size_t j       = 0u;

            for (j = 0u; j < m.count; ++j)
            {
                if (m.values[j] <= x)
                {
                    down = m.values[j];
                }

                if ( (m.values[j] > x ? m.values[j] - x : x - m.values[j]) <
                     (nearest > x ? nearest - x : x - nearest) )
                {
                    nearest = m.values[j];
                }
            }

            if (x > m.values[m.count - 1u])
            {
                down = m.values[m.count - 1u];
            }

            D_TESTS_EXPECT(d_interval_f_clamp(_iv, x) ==
                           ((_iv.step > 0.0f) ? down
                            : ((x < m.values[0]) ? m.values[0]
                               : ((x > m.values[m.count - 1u])
                                     ? m.values[m.count - 1u] : x))),
                           "clamp", _iv.lower, x);

            // (the reference's distances are no use at +inf: inf - inf)
            if ( (_iv.step > 0.0f) &&
                 (x <= FLT_MAX) &&
                 (m.values[m.count - 1u] <= FLT_MAX) )
            {
                D_TESTS_EXPECT(d_interval_f_clamp_nearest(_iv, x) == nearest,
                               "clamp_nearest", _iv.lower, x);
            }
        }
    }

    // the text reads back as the bounds
    {
        char  text[128];
        float lower = 0.0f;
        float upper = 0.0f;
        char  open  = 0;
        char  close = 0;

        (void)d_interval_f_format(text, sizeof(text), _iv);

        if (_iv.step > 0.0f)
        {
            float step = 0.0f;

            D_TESTS_EXPECT( (sscanf(text, "%c%g:%g:%g%c", &open, &lower,
                                    &step, &upper, &close) == 5) &&
                            (lower == _iv.lower) &&
                            (upper == _iv.upper) &&
                            (step == _iv.step),
                            "format reads back", _iv.lower, _iv.upper);
        }
        else
        {
            D_TESTS_EXPECT( (sscanf(text, "%c%g, %g%c", &open, &lower,
                                    &upper, &close) == 4) &&
                            (lower == _iv.lower) &&
                            (upper == _iv.upper),
                            "format reads back", _iv.lower, _iv.upper);
        }
    }

    return;
}

// d_tests_pair
//   the three relations of two intervals against brute force.
static void
d_tests_pair(
    struct d_interval_f _a,
    struct d_interval_f _b
)
{
    struct d_tests_members ma;
    struct d_tests_members mb;
    bool                   shared = false;
    bool                   real   = false;
    size_t                 i      = 0u;

    d_tests_members_of(&ma, _a);
    d_tests_members_of(&mb, _b);

    for (i = 0u; i < ma.count; ++i)
    {
        if (d_tests_members_has(&mb, ma.values[i]))
        {
            shared = true;
        }
    }

    // a common real point is a window float or a midpoint between two
    for (i = 0u; i + 1u < D_TESTS_WINDOW + 8u; ++i)
    {
        const double x   = (double)g_window[i];
        // (between the largest float and +inf, a double past the float's
        // range stands for the open stretch)
        const double mid = (g_window[i + 1u] > FLT_MAX)
                               ? (2.0 * (double)FLT_MAX)
                               : (((double)g_window[i] +
                                   (double)g_window[i + 1u]) / 2.0);
        const double points[2] = { x, mid };
        size_t       p         = 0u;

        for (p = 0u; p < 2u; ++p)
        {
            const double t      = points[p];
            const bool   a_left = ((_a.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
                                      ? (t > _a.lower) : (t >= _a.lower);
            const bool   a_right = ((_a.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
                                      ? (t < _a.upper) : (t <= _a.upper);
            const bool   b_left = ((_b.bounds & D_INTERVAL_LEFT_OPEN) != 0u)
                                      ? (t > _b.lower) : (t >= _b.lower);
            const bool   b_right = ((_b.bounds & D_INTERVAL_RIGHT_OPEN) != 0u)
                                      ? (t < _b.upper) : (t <= _b.upper);

            if (a_left && a_right && b_left && b_right)
            {
                real = true;
            }
        }
    }

    D_TESTS_EXPECT(d_interval_f_intersects(_a, _b) == shared, "intersects",
                   _a.lower, _b.lower);
    D_TESTS_EXPECT(d_interval_f_overlaps_continuous(_a, _b) == real,
                   "overlaps_continuous", _a.lower, _b.lower);

    // overlaps: the inclusive ranges, strides ignored
    {
        const bool ranges =
            ( (!d_interval_f_is_empty(_a)) &&
              (!d_interval_f_is_empty(_b)) &&
              !( (d_interval_f_inclusive_upper(_a) <
                  d_interval_f_inclusive_lower(_b)) ||
                 (d_interval_f_inclusive_lower(_a) >
                  d_interval_f_inclusive_upper(_b)) ) );

        D_TESTS_EXPECT(d_interval_f_overlaps(_a, _b) == ranges, "overlaps",
                       _a.lower, _b.lower);
    }

    return;
}

static void
d_tests_window_run(float _start)
{
    struct d_interval_f ivs[600];
    size_t              n = 0u;
    size_t              i = 0u;
    size_t              j = 0u;

    d_tests_window_fill(_start);

    // every pair of window floats as bounds, every bound kind, five strides
    for (i = 4u; i < 4u + D_TESTS_WINDOW; ++i)
    {
        for (j = i; j < 4u + D_TESTS_WINDOW; ++j)
        {
            unsigned kind = 0u;

            for (kind = 0u; kind < 4u; ++kind)
            {
                const float ulp     = g_window[i + 1u] - g_window[i];
                const float steps[] = { 0.0f, ulp, 2.0f * ulp, 3.0f * ulp };
                size_t      s       = 0u;

                for (s = 0u; s < 4u; ++s)
                {
                    const struct d_interval_f iv =
                        d_interval_f_make(g_window[i], g_window[j], kind,
                                          steps[s]);

                    d_tests_one(iv);

                    // a sample of the intervals for the pair checks
                    if ( (n < 600u) &&
                         ((((i * 7u) + (j * 3u) + kind + s) % 5u) == 0u) )
                    {
                        ivs[n] = iv;
                        ++n;
                    }
                }
            }
        }
    }

    for (i = 0u; i < n; ++i)
    {
        for (j = 0u; j < n; ++j)
        {
            d_tests_pair(ivs[i], ivs[j]);
        }
    }

    return;
}

static void
d_tests_wider_types(void)
{
    // a decimal stride: 0, 0.1, ..., 1 is eleven members, the last exactly 1
    const struct d_interval_d tenths =
        d_interval_d_make(0.0, 1.0, D_INTERVAL_CLOSED, 0.1);
    const struct d_interval_f tenths_f =
        d_interval_f_make(0.0f, 1.0f, D_INTERVAL_CLOSED, 0.1f);
    const struct d_interval_ld tenths_ld =
        d_interval_ld_make(0.0L, 1.0L, D_INTERVAL_CLOSED, 0.1L);
    // a binade's doubles, and an open interval's neighbours
    const struct d_interval_d binade =
        d_interval_d_make(1.0, 2.0, D_INTERVAL_RIGHT_OPEN, 0.0);
    const struct d_interval_d open =
        d_interval_d_make(0.5, 2.5, D_INTERVAL_OPEN, 0.0);
    char text[128];

    D_TESTS_EXPECT(d_interval_d_count(tenths) == 11u, "d tenths count",
                   (double)d_interval_d_count(tenths), 0);
    D_TESTS_EXPECT(d_interval_d_last(tenths) == 1.0, "d tenths end on 1",
                   d_interval_d_last(tenths), 0);
    D_TESTS_EXPECT(d_interval_d_contains(tenths, 0.30000000000000004) &&
                   !d_interval_d_contains(tenths, 0.3),
                   "d tenths hold the computed 3 * 0.1", 0, 0);
    D_TESTS_EXPECT(d_interval_d_index_of(tenths, 0.30000000000000004) == 3u,
                   "d tenths index", 0, 0);
    D_TESTS_EXPECT(d_interval_f_count(tenths_f) == 11u, "f tenths count",
                   (double)d_interval_f_count(tenths_f), 0);
    D_TESTS_EXPECT(d_interval_ld_count(tenths_ld) == 11u, "ld tenths count",
                   (double)d_interval_ld_count(tenths_ld), 0);
    D_TESTS_EXPECT(d_interval_d_count(binade) == ((d_math_umax)1 << 52),
                   "d a binade's doubles", 0, 0);
    D_TESTS_EXPECT(d_interval_d_first(open) == d_math_d_next_up(0.5),
                   "d open first is the neighbour", 0, 0);
    D_TESTS_EXPECT(d_interval_d_clamp(open, 3.0) == d_math_d_next_down(2.5),
                   "d open clamp lands inside", 0, 0);
    D_TESTS_EXPECT(!d_interval_d_contains(open, 0.5) &&
                   d_interval_d_contains(open, d_math_d_next_up(0.5)),
                   "d open bounds exactly", 0, 0);

    (void)d_interval_d_format(text, sizeof(text), tenths);
    D_TESTS_EXPECT(strcmp(text, "[0:0.1:1]") == 0, "d text, shortest", 0, 0);
    (void)d_interval_d_format(text, sizeof(text), open);
    D_TESTS_EXPECT(strcmp(text, "(0.5, 2.5)") == 0, "d text, open", 0, 0);

    // NaN is in no interval, and clamps to NaN
    D_TESTS_EXPECT(!d_interval_d_contains(binade, nan("")), "d NaN", 0, 0);
    D_TESTS_EXPECT(isnan(d_interval_d_clamp(binade, nan(""))), "d NaN clamp",
                   0, 0);

    return;
}

int
main(void)
{
    d_tests_window_run(1.0f);
    d_tests_window_run(-FLT_MIN * FLT_EPSILON * 6.0f);
    d_tests_window_run(FLT_MIN);
    d_tests_window_run(-3.0f);
    d_tests_window_run(nextafterf(FLT_MAX, 0.0f));
    d_tests_wider_types();

    printf("t_interval_float: %ld checks, %ld failures\n", g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
