/*******************************************************************************
* djinterp [c/math]                                                    t_coord.c
*
*   Conformance harness for c/math/coord.h, all three families. The library
* forms against the coordinate headers' former formulas, written out here in
* the same order on the C library's functions, bit for bit; each _c form
* near its library form; and the geometry the formulas must keep: a round
* trip through cartesian returns the point (once canonical), the law of
* cosines agrees with the cartesian distance, the angular distance with the
* angle between the unit vectors, the canonical forms land in their ranges,
* the origin converts to the origin.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_coord.c -o t_coord -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_coord.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.11.07
*******************************************************************************/
// std
#include <math.h>    // sqrt, sin, cos, atan2, acos, fmod
#include <stdio.h>   // printf
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/coord.h"  // unit under test


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 8675309u;

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

// one family's test: T its type, S its suffix, L the C library's suffix (f,
// nothing, l), E a tolerance for what holds only to rounding
#define D_TESTS_COORD_FAMILY(T, S, SQRT, SIN, COS, ATAN2, ACOS, E)             \
static bool                                                                    \
d_tests_near_##S(T _a, T _b, T _tol)                                           \
{                                                                              \
    const T d = _a - _b;                                                       \
                                                                               \
    return ( (d <= _tol) &&                                                    \
             (d >= -_tol) );                                                   \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_coord_##S(void)                                                        \
{                                                                              \
    const T pi = (T)3.14159265358979323846L;                                   \
    long    trial = 0;                                                         \
                                                                               \
    for (trial = 0; trial < 100000; ++trial)                                   \
    {                                                                          \
        T p[3], c[3], q[3], b[3], w[3];                                        \
        T d = (T)0;                                                            \
                                                                               \
        p[0] = (T)(d_tests_random() + 4.5);                                    \
        p[1] = (T)(d_tests_random() * 0.75);                                   \
        p[2] = (T)d_tests_random();                                            \
        b[0] = (T)(d_tests_random() + 4.5);                                    \
        b[1] = (T)(d_tests_random() * 0.75);                                   \
        b[2] = (T)d_tests_random();                                            \
                                                                               \
        /* the library forms are the former formulas, in their order */        \
        /* (every step of a reference stored: an x87 rounds a whole       */   \
        /* expression once, at the end)                                   */   \
        d_coord_##S##_polar_to_cartesian(c, p);                                \
        {                                                                      \
            volatile T c0 = COS(p[1]), s0 = SIN(p[1]);                         \
            volatile T x0 = p[0] * c0, y0 = p[0] * s0;                         \
            D_TESTS_EXPECT( (c[0] == x0) && (c[1] == y0),                      \
                            #S " polar to cartesian");                         \
        }                                                                      \
        d_coord_##S##_polar_from_cartesian(q, c);                              \
        {                                                                      \
            volatile T xx = c[0] * c[0], yy = c[1] * c[1];                     \
            volatile T sum = xx + yy;                                          \
            volatile T r0 = SQRT(sum), a0 = ATAN2(c[1], c[0]);                 \
            D_TESTS_EXPECT( (q[0] == r0) && (q[1] == a0),                      \
                            #S " polar from cartesian");                       \
        }                                                                      \
        d_coord_##S##_spherical_to_cartesian(c, p);                            \
        {                                                                      \
            volatile T st = SIN(p[1]), ct = COS(p[1]);                         \
            volatile T cp = COS(p[2]), sp = SIN(p[2]);                         \
            volatile T rst = p[0] * st;                                        \
            volatile T x0 = rst * cp, y0 = rst * sp, z0 = p[0] * ct;           \
            D_TESTS_EXPECT( (c[0] == x0) && (c[1] == y0) && (c[2] == z0),      \
                            #S " spherical to cartesian");                     \
        }                                                                      \
        d_coord_##S##_spherical_from_cartesian(q, c);                          \
        {                                                                      \
            volatile T xx = c[0] * c[0], yy = c[1] * c[1], zz = c[2] * c[2];   \
            volatile T xy = xx + yy;                                           \
            volatile T sum = xy + zz;                                          \
            volatile T r = SQRT(sum), rho = SQRT(xy);                          \
            volatile T t0 = ATAN2(rho, c[2]), f0 = ATAN2(c[1], c[0]);          \
            D_TESTS_EXPECT( (q[0] == r) && (q[1] == t0) && (q[2] == f0),       \
                            #S " spherical from cartesian");                   \
        }                                                                      \
                                                                               \
        /* the _c forms near the library ones */                               \
        d_coord_##S##_spherical_to_cartesian_c(w, p);                          \
        D_TESTS_EXPECT( (d_tests_near_##S(w[0], c[0], (T)E)) &&                \
                        (d_tests_near_##S(w[1], c[1], (T)E)) &&                \
                        (d_tests_near_##S(w[2], c[2], (T)E)),                  \
                        #S " spherical_to_cartesian_c near the library's");    \
        d_coord_##S##_spherical_from_cartesian_c(w, c);                        \
        D_TESTS_EXPECT( (d_tests_near_##S(w[0], q[0], (T)E)) &&                \
                        (d_tests_near_##S(w[1], q[1], (T)E)) &&                \
                        (d_tests_near_##S(w[2], q[2], (T)E)),                  \
                        #S " spherical_from_cartesian_c near the library's");  \
                                                                               \
        /* round trips, once canonical */                                      \
        d_coord_##S##_spherical_canonicalize(w, p);                            \
        d_coord_##S##_spherical_to_cartesian(c, w);                            \
        d_coord_##S##_spherical_from_cartesian(q, c);                          \
        d_coord_##S##_spherical_canonicalize(q, q);                            \
        /* (phi only away from the poles, where it is ill-conditioned: */      \
        /* its error grows as 1 / sin(theta))                              */  \
        D_TESTS_EXPECT( (d_tests_near_##S(q[0], w[0], (T)(E * 16))) &&         \
                        (d_tests_near_##S(q[1], w[1], (T)(E * 16))) &&         \
                        ( (SIN(w[1]) < (T)0.01) ||                             \
                          (d_tests_near_##S(q[2], w[2], (T)(E * 1600))) ||     \
                          (d_tests_near_##S(q[2] + (T)2 * pi, w[2],            \
                                            (T)(E * 1600))) ||                 \
                          (d_tests_near_##S(q[2], w[2] + (T)2 * pi,            \
                                            (T)(E * 1600))) ),                 \
                        #S " spherical round trip");                           \
        D_TESTS_EXPECT( (w[0] >= (T)0) &&                                      \
                        (w[1] >= (T)0) && (w[1] <= pi) &&                      \
                        (w[2] >= (T)0) && (w[2] < (T)2 * pi),                  \
                        #S " the spherical canonical ranges");                 \
        d_coord_##S##_cylindrical_to_spherical(q, p);                          \
        d_coord_##S##_spherical_to_cylindrical(w, q);                          \
        D_TESTS_EXPECT( (d_tests_near_##S(w[0], p[0], (T)(E * 16))) &&         \
                        (w[1] == p[1]) &&                                      \
                        (d_tests_near_##S(w[2], p[2], (T)(E * 16))),           \
                        #S " cylindrical to spherical and back");              \
                                                                               \
        /* distances: the law of cosines is the cartesian distance */          \
        d = d_coord_##S##_cylindrical_distance(p, b);                          \
        d_coord_##S##_cylindrical_to_cartesian(c, p);                          \
        d_coord_##S##_cylindrical_to_cartesian(q, b);                          \
        {                                                                      \
            const T dx = c[0] - q[0], dy = c[1] - q[1], dz = c[2] - q[2];      \
            D_TESTS_EXPECT(d_tests_near_##S(d, SQRT(dx * dx + dy * dy +        \
                                                    dz * dz), (T)(E * 64)),    \
                           #S " the cylindrical law of cosines");              \
        }                                                                      \
        {                                                                      \
            const T pa[3] = { p[0], p[1], (T)0 };                              \
            const T pb[3] = { b[0], b[1], (T)0 };                              \
            D_TESTS_EXPECT(d_tests_near_##S(                                   \
                               d_coord_##S##_polar_distance(p, b),             \
                               d_coord_##S##_cylindrical_distance(pa, pb),     \
                               (T)(E * 4)),                                    \
                           #S " polar distance is the plane's");               \
        }                                                                      \
                                                                               \
        /* the angular distance is the angle between the unit vectors */       \
        {                                                                      \
            T ua[3], ub[3];                                                    \
            const T one_a[3] = { (T)1, p[1], p[2] };                           \
            const T one_b[3] = { (T)1, b[1], b[2] };                           \
            T dot = (T)0;                                                      \
            d_coord_##S##_spherical_to_cartesian(ua, one_a);                   \
            d_coord_##S##_spherical_to_cartesian(ub, one_b);                   \
            dot = ua[0] * ub[0] + ua[1] * ub[1] + ua[2] * ub[2];               \
            dot = (dot > (T)1) ? (T)1 : ((dot < (T)-1) ? (T)-1 : dot);         \
            D_TESTS_EXPECT(d_tests_near_##S(                                   \
                               d_coord_##S##_spherical_angular_distance(       \
                                   one_a, one_b),                              \
                               ACOS(dot), (T)(E * 4096)),                      \
                           #S " the angular distance");                        \
        }                                                                      \
                                                                               \
        /* angles: exact, so the formula's own value */                        \
        {                                                                      \
            const T a       = (T)(d_tests_random() * 8.0);                     \
            const T two_pi  = (T)2 * (T)3.14159265358979323846L;               \
            T       want    = (T)fmod((double)a, (double)two_pi);              \
            want = (want < (T)0) ? (want + two_pi) : want;                     \
            D_TESTS_EXPECT( (sizeof(T) != sizeof(double)) ||                   \
                            (d_coord_##S##_normalize_angle(a) == want),        \
                            #S " normalize_angle is the former formula");      \
            D_TESTS_EXPECT( (d_coord_##S##_normalize_angle(a) >= (T)0) &&      \
                            (d_coord_##S##_normalize_angle(a) < two_pi),       \
                            #S " a normalized angle is in [0, 2 pi)");         \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* the origin, and a negative radius */                                    \
    {                                                                          \
        const T zero[3] = { (T)0, (T)0, (T)0 };                                \
        const T neg[3]  = { (T)-2, (T)0.5, (T)1 };                             \
        T       out[3]  = { (T)9, (T)9, (T)9 };                                \
                                                                               \
        d_coord_##S##_spherical_from_cartesian(out, zero);                     \
        D_TESTS_EXPECT( (out[0] == (T)0) && (out[1] == (T)0) &&                \
                        (out[2] == (T)0),                                      \
                        #S " the origin converts to the origin");              \
        d_coord_##S##_polar_canonicalize(out, neg);                            \
        D_TESTS_EXPECT( (out[0] == (T)2) &&                                    \
                        (d_tests_near_##S(out[1], (T)0.5 + pi, (T)E)),         \
                        #S " a negative radius, a half turn round");           \
        d_coord_##S##_cylindrical_canonicalize(out, neg);                      \
        D_TESTS_EXPECT( (out[0] == (T)2) && (out[2] == (T)1),                  \
                        #S " cylindrical canonical form");                     \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_COORD_FAMILY(float,       f,  sqrtf, sinf, cosf, atan2f, acosf, 1e-5)
D_TESTS_COORD_FAMILY(double,      d,  sqrt,  sin,  cos,  atan2,  acos,  1e-13)
D_TESTS_COORD_FAMILY(long double, ld, sqrtl, sinl, cosl, atan2l, acosl, 1e-16)

int
main(void)
{
    d_tests_coord_f();
    d_tests_coord_d();
    d_tests_coord_ld();

    printf("t_coord: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
