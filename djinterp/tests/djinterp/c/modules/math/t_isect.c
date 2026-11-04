/*******************************************************************************
* djinterp [c/math]                                                    t_isect.c
*
*   Conformance harness for c/math/isect.h, all three families: each test
* against the geometry headers' former formulas, written out here in the
* same order, on 200,000 random rays per family -- the same answer and the
* same parameters, bit for bit -- and the geometry the tests must keep: a
* hit's parameter lands the ray on the box's surface or in the triangle's
* plane, inside it; a ray from inside a box enters behind its origin; a box
* behind the ray, a ray parallel to a slab outside it, and a ray parallel
* to a triangle all miss.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_isect.c -o t_isect -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_isect.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
// std
#include <math.h>    // HUGE_VAL
#include <stdio.h>   // printf
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/isect.h"  // unit under test


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 1618033u;

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

// one family's test: T its type, S its suffix, E a tolerance
#define D_TESTS_ISECT_FAMILY(T, S, E)                                          \
/* the slab test as ray.hpp wrote it */                                        \
static bool                                                                    \
d_tests_slab_##S(const T* o, const T* d, const T* lo, const T* hi,             \
                 T* tn, T* tf)                                                 \
{                                                                              \
    T t_min = -(T)HUGE_VAL, t_max = (T)HUGE_VAL;                               \
    int i = 0;                                                                 \
    for (i = 0; i < 3; ++i)                                                    \
    {                                                                          \
        if ((d[i] > -(T)1e-12) && (d[i] < (T)1e-12))                           \
        {                                                                      \
            if ((o[i] < lo[i]) || (o[i] > hi[i])) { return false; }            \
            continue;                                                          \
        }                                                                      \
        {                                                                      \
            /* (one operation per statement: an x87 rounds each only    */    \
            /* when it is stored)                                       */    \
            const T inv = (T)1 / d[i];                                         \
            const T to_lo = lo[i] - o[i], to_hi = hi[i] - o[i];                \
            T t1 = to_lo * inv, t2 = to_hi * inv;                              \
            if (t1 > t2) { const T x = t1; t1 = t2; t2 = x; }                  \
            if (t1 > t_min) { t_min = t1; }                                    \
            if (t2 < t_max) { t_max = t2; }                                    \
            if (t_min > t_max) { return false; }                               \
        }                                                                      \
    }                                                                          \
    if (t_max < (T)0) { return false; }                                        \
    *tn = t_min; *tf = t_max;                                                  \
    return true;                                                               \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_isect_##S(void)                                                        \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 200000; ++trial)                                   \
    {                                                                          \
        T o[3], d[3], lo[3], hi[3], v0[3], v1[3], v2[3];                       \
        T tn = (T)0, tf = (T)0, wn = (T)0, wf = (T)0, t = (T)0;                \
        bool hit = false, want = false;                                        \
        int i = 0;                                                             \
                                                                               \
        for (i = 0; i < 3; ++i)                                                \
        {                                                                      \
            const T a = (T)d_tests_random(), b = (T)d_tests_random();          \
            o[i]  = (T)(d_tests_random() * 2.0);                               \
            d[i]  = (trial % 7 == i) ? (T)0 : (T)d_tests_random();             \
            lo[i] = (a < b) ? a : b;                                           \
            hi[i] = (a < b) ? b : a;                                           \
            v0[i] = (T)d_tests_random();                                       \
            v1[i] = (T)d_tests_random();                                       \
            v2[i] = (T)d_tests_random();                                       \
        }                                                                      \
                                                                               \
        /* the box: the former formula's answer and parameters */              \
        hit  = d_isect_##S##_ray_aabb(o, d, lo, hi, &tn, &tf);                 \
        want = d_tests_slab_##S(o, d, lo, hi, &wn, &wf);                       \
        D_TESTS_EXPECT( (hit == want) &&                                       \
                        ((!hit) || ((tn == wn) && (tf == wf))),                \
                        #S " ray_aabb is the former slab test");               \
                                                                               \
        /* a hit lands the ray on the box, between the slabs */                \
        if (hit && (tf >= (T)0))                                               \
        {                                                                      \
            const T s = (tn > (T)0) ? tn : tf;                                 \
            bool    inside = true;                                             \
            for (i = 0; i < 3; ++i)                                            \
            {                                                                  \
                const T p = o[i] + (d[i] * s);                                 \
                inside = inside && (p >= lo[i] - (T)E) && (p <= hi[i] + (T)E); \
            }                                                                  \
            D_TESTS_EXPECT(inside, #S " a box hit lands on the box");          \
        }                                                                      \
                                                                               \
        /* the triangle: a hit lies in its plane, inside it */                 \
        if (d_isect_##S##_ray_triangle(o, d, v0, v1, v2, (T)1e-9, &t))         \
        {                                                                      \
            T p[3], e1[3], e2[3], n[3], w[3];                                  \
            for (i = 0; i < 3; ++i)                                            \
            {                                                                  \
                p[i]  = o[i] + (d[i] * t);                                     \
                e1[i] = v1[i] - v0[i];                                         \
                e2[i] = v2[i] - v0[i];                                         \
                w[i]  = p[i] - v0[i];                                          \
            }                                                                  \
            d_vec_##S##_cross(n, e1, e2);                                      \
            D_TESTS_EXPECT( (t > (T)1e-9) &&                                   \
                            (d_vec_##S##_dot(n, w, 3u) <                       \
                             (T)E * (T)256 * d_vec_##S##_norm(n, 3u) *         \
                             ((T)1 + d_vec_##S##_norm(w, 3u))) &&              \
                            (d_vec_##S##_dot(n, w, 3u) >                       \
                             -(T)E * (T)256 * d_vec_##S##_norm(n, 3u) *        \
                             ((T)1 + d_vec_##S##_norm(w, 3u))),                \
                            #S " a triangle hit lies in its plane");           \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* the cases: inside, behind, parallel */                                  \
    {                                                                          \
        const T lo[3] = { (T)-1, (T)-1, (T)-1 };                               \
        const T hi[3] = { (T)1, (T)1, (T)1 };                                  \
        const T in[3] = { (T)0, (T)0, (T)0 };                                  \
        const T ahead[3] = { (T)-5, (T)0, (T)0 };                              \
        const T x[3] = { (T)1, (T)0, (T)0 };                                   \
        const T nx[3] = { (T)-1, (T)0, (T)0 };                                 \
        const T up[3] = { (T)0, (T)5, (T)0 };                                  \
        const T t0[3] = { (T)0, (T)0, (T)0 };                                  \
        const T t1[3] = { (T)0, (T)1, (T)0 };                                  \
        const T t2[3] = { (T)0, (T)0, (T)1 };                                  \
        const T yz[3] = { (T)0, (T)0.25, (T)0.25 };                            \
        T tn = (T)0, tf = (T)0, t = (T)0;                                      \
                                                                               \
        D_TESTS_EXPECT( (d_isect_##S##_ray_aabb(in, x, lo, hi, &tn, &tf)) &&   \
                        (tn == (T)-1) && (tf == (T)1),                         \
                        #S " from inside, the box entered behind");            \
        D_TESTS_EXPECT( (d_isect_##S##_ray_aabb(ahead, x, lo, hi, &tn, &tf)) &&\
                        (tn == (T)4) && (tf == (T)6),                          \
                        #S " a box ahead");                                    \
        D_TESTS_EXPECT(!d_isect_##S##_ray_aabb(ahead, nx, lo, hi, &tn, &tf),   \
                       #S " a box behind");                                    \
        D_TESTS_EXPECT(!d_isect_##S##_ray_aabb(up, x, lo, hi, &tn, &tf),       \
                       #S " parallel to a slab, outside it");                  \
        /* through a vertex: a hit (the edges are inclusive); in the plane: */ \
        /* parallel, a miss                                                 */ \
        D_TESTS_EXPECT( (d_isect_##S##_ray_triangle(ahead, x, t0, t1, t2,      \
                                                    (T)1e-9, &t)) &&           \
                        (t == (T)5) &&                                         \
                        (d_isect_##S##_ray_triangle(t1, t1, t0, t1, t2,        \
                                                    (T)1e-9, &t) == false),    \
                        #S " through a vertex, and in the plane");             \
        {                                                                      \
            const T from[3] = { (T)-2, (T)0.25, (T)0.25 };                     \
            D_TESTS_EXPECT( (d_isect_##S##_ray_triangle(from, x, t0, t1, t2,   \
                                                        (T)1e-9, &t)) &&       \
                            (t == (T)2),                                       \
                            #S " a triangle straight ahead");                  \
            (void)yz;                                                          \
        }                                                                      \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_ISECT_FAMILY(float,       f,  1e-5)
D_TESTS_ISECT_FAMILY(double,      d,  1e-13)
D_TESTS_ISECT_FAMILY(long double, ld, 1e-16)

int
main(void)
{
    d_tests_isect_f();
    d_tests_isect_d();
    d_tests_isect_ld();

    printf("t_isect: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
