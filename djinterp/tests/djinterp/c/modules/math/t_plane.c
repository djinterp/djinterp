/*******************************************************************************
* djinterp [c/math]                                                    t_plane.c
*
*   Conformance harness for c/math/plane.h, all three families: each kernel
* against the 2D shape header's former formulas, written out here in the
* same order with every step stored, on 100,000 random polygons and points
* per family -- the same answers, bit for bit -- and the geometry: a square
* contains its interior and not its outside, a rectangle's area and a
* triangle's centroid are what they are, a polygon's area does not depend on
* where its vertex list starts, and a degenerate polygon has no centroid.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_plane.c -o t_plane -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_plane.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
// std
#include <stdio.h>   // printf
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/plane.h"  // unit under test


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 1732050u;

static void
d_tests_expect(
    bool        _ok,
    const char* _what,
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
            printf("  FAIL line %d: %s\n", _line, _what);
        }
    }

    return;
}

#define D_TESTS_EXPECT(ok, what)                                              \
    d_tests_expect((ok), (what), __LINE__)

static double
d_tests_random(void)
{
    g_state = (g_state * 1103515245u) + 12345u;

    return ((double)((g_state >> 4) % 2000003u) / 250000.0) - 4.0;
}

// one family's test: T its type, S its suffix
#define D_TESTS_PLANE_FAMILY(T, S)                                             \
/* the former formulas, every step stored */                                   \
static bool                                                                    \
d_tests_crosses_##S(const T* p, const T* a, const T* b)                        \
{                                                                              \
    if ((a[1] > p[1]) != (b[1] > p[1]))                                        \
    {                                                                          \
        volatile T dx = b[0] - a[0], dy = p[1] - a[1], den = b[1] - a[1];      \
        volatile T run = dx * dy;                                              \
        volatile T off = run / den;                                            \
        volatile T x = off + a[0];                                             \
        return (p[0] < x);                                                     \
    }                                                                          \
    return false;                                                              \
}                                                                              \
                                                                               \
static T                                                                       \
d_tests_cross_##S(const T* a, const T* b)                                      \
{                                                                              \
    volatile T first = a[0] * b[1], second = b[0] * a[1];                      \
    volatile T d = first - second;                                             \
    return d;                                                                  \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_plane_##S(void)                                                        \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 100000; ++trial)                                   \
    {                                                                          \
        T      xy[16], p[2], c[2] = { (T)0, (T)0 };                            \
        size_t n = (size_t)(3 + (trial % 6)), i = 0, j = 0;                    \
        size_t crossings = 0;                                                  \
        T      sum = (T)0, cx = (T)0, cy = (T)0;                               \
                                                                               \
        for (i = 0; i < 2 * n; ++i)                                            \
        {                                                                      \
            xy[i] = (T)d_tests_random();                                       \
        }                                                                      \
        p[0] = (T)d_tests_random();                                            \
        p[1] = (T)d_tests_random();                                            \
                                                                               \
        /* the former loops */                                                 \
        for (i = 0, j = n - 1; i < n; j = i++)                                 \
        {                                                                      \
            crossings += d_tests_crosses_##S(p, xy + 2 * i, xy + 2 * j)        \
                             ? 1 : 0;                                          \
        }                                                                      \
        for (i = 0; i < n; ++i)                                                \
        {                                                                      \
            const size_t k = (i + 1) % n;                                      \
            const T      cr = d_tests_cross_##S(xy + 2 * i, xy + 2 * k);       \
            volatile T   sx = xy[2 * i] + xy[2 * k];                           \
            volatile T   sy = xy[2 * i + 1] + xy[2 * k + 1];                   \
            volatile T   tx = sx * cr, ty = sy * cr;                           \
            sum += cr;                                                         \
            cx  += tx;                                                         \
            cy  += ty;                                                         \
        }                                                                      \
                                                                               \
        D_TESTS_EXPECT(d_plane_##S##_polygon_contains(xy, n, p) ==             \
                       ((crossings % 2) == 1),                                 \
                       #S " polygon_contains is the former loop");             \
        {                                                                      \
            volatile T half = sum / (T)2;                                      \
            const T    area = (half < (T)0) ? -half : half;                    \
            D_TESTS_EXPECT(d_plane_##S##_polygon_area(xy, n) == area,          \
                           #S " polygon_area is the former shoelace sum");     \
        }                                                                      \
        if (d_plane_##S##_polygon_centroid(c, xy, n))                          \
        {                                                                      \
            volatile T den = (T)3 * sum;                                       \
            volatile T wx = cx / den, wy = cy / den;                           \
            D_TESTS_EXPECT( (c[0] == wx) && (c[1] == wy),                      \
                            #S " polygon_centroid is the former formula");     \
        }                                                                      \
                                                                               \
        /* the area does not depend on where the list starts (to rounding) */  \
        {                                                                      \
            T      rot[16];                                                    \
            const T a0 = d_plane_##S##_polygon_area(xy, n);                    \
            for (i = 0; i < 2 * n; ++i)                                        \
            {                                                                  \
                rot[i] = xy[(i + 2) % (2 * n)];                                \
            }                                                                  \
            D_TESTS_EXPECT(                                                    \
                ((d_plane_##S##_polygon_area(rot, n) - a0) <                   \
                 (T)1e-3 * ((T)1 + a0)) &&                                     \
                ((d_plane_##S##_polygon_area(rot, n) - a0) >                   \
                 -(T)1e-3 * ((T)1 + a0)),                                      \
                #S " the area of a rotated vertex list");                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* the cases */                                                            \
    {                                                                          \
        const T sq[8]  = { (T)0, (T)0, (T)2, (T)0, (T)2, (T)2, (T)0, (T)2 };   \
        const T in[2]  = { (T)1, (T)1 };                                       \
        const T out[2] = { (T)3, (T)1 };                                       \
        const T tri[6] = { (T)0, (T)0, (T)3, (T)0, (T)0, (T)3 };               \
        const T line[6] = { (T)0, (T)0, (T)1, (T)1, (T)2, (T)2 };              \
        T       c[2] = { (T)7, (T)7 };                                         \
                                                                               \
        D_TESTS_EXPECT( (d_plane_##S##_polygon_contains(sq, 4u, in)) &&        \
                        (!d_plane_##S##_polygon_contains(sq, 4u, out)) &&      \
                        (d_plane_##S##_polygon_area(sq, 4u) == (T)4),          \
                        #S " a square");                                       \
        D_TESTS_EXPECT( (d_plane_##S##_polygon_centroid(c, tri, 3u)) &&        \
                        (c[0] == (T)1) && (c[1] == (T)1),                      \
                        #S " a triangle's centroid");                          \
        c[0] = (T)7;                                                           \
        D_TESTS_EXPECT( (!d_plane_##S##_polygon_centroid(c, line, 3u)) &&      \
                        (c[0] == (T)7),                                        \
                        #S " a degenerate polygon: no centroid");              \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_PLANE_FAMILY(float,       f)
D_TESTS_PLANE_FAMILY(double,      d)
D_TESTS_PLANE_FAMILY(long double, ld)

int
main(void)
{
    d_tests_plane_f();
    d_tests_plane_d();
    d_tests_plane_ld();

    printf("t_plane: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
