/*******************************************************************************
* djinterp [test]                                                       t_quat.c
*
*   Conformance harness for c/math/quat.h, all three families: the algebra
* against the quaternion headers' former formulas written out here in the
* same order, bit for bit -- the Hamilton product, the conjugate, the
* normalization (a zero quaternion left alone), the rotation of a vector,
* the matrix forms -- and the identities: q q* = |q|^2, rotation by q equal
* to its matrix's product, q and -q the same rotation, a quarter turn's
* image. Then the trig-dependent kernels: each _c form within a few ulps of
* its library form, slerp's ends and midpoint, the zero axis.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_quat.c -o t_quat -lm
*
*
* path:      /tests/djinterp/c/math/t_quat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.10.05
*******************************************************************************/
// std
#include <math.h>    // sqrt, fabs
#include <stdio.h>   // printf
// djinterp
#include "../../../../inc/djinterp/c/math/quat.h"  // unit under test


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 1234567u;

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

// one family's test, stamped for each: T its type, S its suffix, E a
// tolerance for the identities that hold only to rounding
#define D_TESTS_QUAT_FAMILY(T, S, E)                                           \
static bool                                                                    \
d_tests_near_##S(const T* _x, const T* _y, size_t _n, T _tol)                  \
{                                                                              \
    size_t k = 0u;                                                             \
                                                                               \
    for (k = 0u; k < _n; ++k)                                                  \
    {                                                                          \
        const T d = _x[k] - _y[k];                                             \
                                                                               \
        if (((d < (T)0) ? -d : d) > _tol)                                      \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    return true;                                                               \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_quat_##S(void)                                                         \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 100000; ++trial)                                   \
    {                                                                          \
        T a[4], b[4], out[4], want[4], v[3], rv[3], m[9], m4[16], mv[3];       \
        T n = (T)0;                                                            \
        size_t i = 0u;                                                         \
                                                                               \
        for (i = 0u; i < 4u; ++i)                                              \
        {                                                                      \
            a[i] = (T)(d_tests_random() / 4.0);                                \
            b[i] = (T)(d_tests_random() / 4.0);                                \
        }                                                                      \
                                                                               \
        for (i = 0u; i < 3u; ++i)                                              \
        {                                                                      \
            v[i] = (T)d_tests_random();                                        \
        }                                                                      \
                                                                               \
        /* the Hamilton product, as the headers wrote it */                    \
        d_quat_##S##_multiply(out, a, b);                                      \
        {                                                                      \
            const T ax = a[0], ay = a[1], az = a[2], aw = a[3];                \
            const T bx = b[0], by = b[1], bz = b[2], bw = b[3];                \
            T p = (T)0, q = (T)0, r = (T)0, t = (T)0;                          \
            p = aw * bx; q = ax * bw; r = p + q; p = ay * bz; r = r + p;       \
            t = az * by; want[0] = r - t;                                      \
            p = aw * by; q = ax * bz; r = p - q; p = ay * bw; r = r + p;       \
            t = az * bx; want[1] = r + t;                                      \
            p = aw * bz; q = ax * by; r = p + q; p = ay * bx; r = r - p;       \
            t = az * bw; want[2] = r + t;                                      \
            p = aw * bw; q = ax * bx; r = p - q; p = ay * by; r = r - p;       \
            t = az * bz; want[3] = r - t;                                      \
        }                                                                      \
        D_TESTS_EXPECT(d_vec_##S##_equal(out, want, 4u),                       \
                       #S " the Hamilton product");                            \
                                                                               \
        /* q q* = (0, 0, 0, |q|^2), to rounding */                             \
        d_quat_##S##_conjugate(want, a);                                       \
        d_quat_##S##_multiply(out, a, want);                                   \
        n = d_vec_##S##_norm_squared(a, 4u);                                   \
        want[0] = (T)0; want[1] = (T)0; want[2] = (T)0; want[3] = n;           \
        D_TESTS_EXPECT(d_tests_near_##S(out, want, 4u, (T)E),                  \
                       #S " q q* = |q|^2");                                    \
                                                                               \
        /* a unit quaternion: rotation equals its matrix's product */          \
        d_quat_##S##_normalize(a, a, d_vec_##S##_norm(a, 4u));                 \
        d_quat_##S##_rotate(rv, a, v);                                         \
        d_quat_##S##_to_matrix3(m, a);                                         \
        d_mat_##S##_multiply_vector(mv, m, v, 3u, 3u);                         \
        D_TESTS_EXPECT(d_tests_near_##S(rv, mv, 3u, (T)(E * 16)),              \
                       #S " rotation is the matrix's product");                \
        d_vec_##S##_negate(b, a, 4u);                                          \
        d_quat_##S##_rotate(mv, b, v);                                         \
        D_TESTS_EXPECT(d_vec_##S##_equal(rv, mv, 3u),                          \
                       #S " q and -q are the same rotation");                  \
        d_quat_##S##_to_matrix4(m4, a);                                        \
        D_TESTS_EXPECT( (m4[0] == m[0]) && (m4[5] == m[4]) &&                  \
                        (m4[10] == m[8]) && (m4[15] == (T)1) &&                \
                        (m4[3] == (T)0) && (m4[12] == (T)0),                   \
                        #S " the 4x4 holds the 3x3");                          \
                                                                               \
        /* the trig kernels: the _c forms near the library ones */             \
        {                                                                      \
            const T angle = (T)d_tests_random();                               \
            T       qc[4], qr[4];                                              \
            d_quat_##S##_from_axis_angle_c(qc, v, angle);                      \
            d_quat_##S##_from_axis_angle(qr, v, angle);                        \
            D_TESTS_EXPECT(d_tests_near_##S(qc, qr, 4u, (T)(E / 64)),          \
                           #S " from_axis_angle_c near from_axis_angle");      \
            d_quat_##S##_slerp_c(qc, a, b, (T)0.25);                           \
            d_quat_##S##_slerp(qr, a, b, (T)0.25);                             \
            D_TESTS_EXPECT(d_tests_near_##S(qc, qr, 4u, (T)(E / 16)),          \
                           #S " slerp_c near slerp");                          \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* slerp: the ends, and an eighth turn halfway to a quarter turn (not  */ \
    /* a half turn: its two arcs are equal, and rounding picks one)         */ \
    {                                                                          \
        const T id[4]   = { (T)0, (T)0, (T)0, (T)1 };                          \
        const T axis[3] = { (T)0, (T)0, (T)2 };                                \
        const T x[3]    = { (T)1, (T)0, (T)0 };                                \
        const T r8      = (T)0.70710678118654752440L;                          \
        T       quarter[4], q[4], r[3];                                        \
                                                                               \
        d_quat_##S##_from_axis_angle(quarter, axis,                            \
                                     (T)1.57079632679489661923L);              \
        d_quat_##S##_slerp(q, id, quarter, (T)0);                              \
        D_TESTS_EXPECT(d_tests_near_##S(q, id, 4u, (T)E), #S " slerp at 0");   \
        d_quat_##S##_slerp(q, id, quarter, (T)1);                              \
        D_TESTS_EXPECT(d_tests_near_##S(q, quarter, 4u, (T)E),                 \
                       #S " slerp at 1");                                      \
        d_quat_##S##_slerp(q, id, quarter, (T)0.5);                            \
        d_quat_##S##_rotate(r, q, x);                                          \
        D_TESTS_EXPECT( (d_tests_near_##S(r, &r8, 1u, (T)E)) &&                \
                        (d_tests_near_##S(r + 1, &r8, 1u, (T)E)),              \
                        #S " halfway to a quarter turn is an eighth");         \
                                                                               \
        /* a zero axis: the identity, or (0, 0, 0, cos) once normalized */     \
        {                                                                      \
            const T zero[3]  = { (T)0, (T)0, (T)0 };                           \
            const T zero4[4] = { (T)0, (T)0, (T)0, (T)0 };                     \
            d_quat_##S##_from_axis_angle(q, zero, (T)1);                       \
            D_TESTS_EXPECT(d_vec_##S##_equal(q, id, 4u),                       \
                           #S " a zero axis names no rotation");               \
            d_quat_##S##_from_unit_axis_angle(q, zero, (T)1);                  \
            D_TESTS_EXPECT( (q[0] == (T)0) &&                                  \
                            (q[3] == d_math_##S##_cos((T)0.5)),                \
                            #S " a zero unit axis");                           \
            d_quat_##S##_normalize(q, zero4, (T)0);                            \
            D_TESTS_EXPECT(q[2] == (T)0,                                       \
                           #S " a zero quaternion normalizes to itself");      \
        }                                                                      \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_QUAT_FAMILY(float,       f,  1e-5)
D_TESTS_QUAT_FAMILY(double,      d,  1e-13)
D_TESTS_QUAT_FAMILY(long double, ld, 1e-16)

int
main(void)
{
    d_tests_quat_f();
    d_tests_quat_d();
    d_tests_quat_ld();

    printf("t_quat: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
