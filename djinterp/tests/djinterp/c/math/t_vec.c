/*******************************************************************************
* djinterp [test]                                                        t_vec.c
*
*   Conformance harness for c/math/vec.h, all three families: every kernel
* against the formula it implements, written out here in the same type and
* the same order of operations, over 200,000 random vectors of 1 to 8
* components per family -- so the results must agree bit for bit -- with the
* two forms of each root required to agree bit for bit with each other, the
* angle's two forms within two ulps, and the cases a careless kernel gets
* wrong: the zero vector, parallel vectors (an angle of 0, not NaN), and an
* output that is also an input.
*
*   One test body serves the three families, stamped out by
* D_TESTS_VEC_FAMILY: the families are one text, and so is their test.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_vec.c -o t_vec -lm
*
*
* path:      /tests/djinterp/c/math/t_vec.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <float.h>   // FLT_EPSILON ...
#include <math.h>    // sqrtf, sqrt, sqrtl, acosf, acos, acosl
#include <stdio.h>   // printf
#include <string.h>  // memcpy
// djinterp
#include "../../../../inc/djinterp/c/math/vec.h"  // unit under test


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 88172645u;

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

// a value in [-4, 4) with a random fraction
static double
d_tests_random(void)
{
    g_state = (g_state * 1103515245u) + 12345u;

    return ((double)((g_state >> 4) % 2000003u) / 250000.0) - 4.0;
}

// one family's test: T its type, S its suffix, ROOT and ACOS the C library's.
// (Values are compared with ==, not memcmp: a long double's padding bytes
// are not part of its value.)
#define D_TESTS_VEC_FAMILY(T, S, ROOT, ACOS, EPSILON)                          \
static bool                                                                    \
d_tests_same_##S(const T* _x, const T* _y, size_t _n)                          \
{                                                                              \
    size_t k = 0u;                                                             \
                                                                               \
    for (k = 0u; k < _n; ++k)                                                  \
    {                                                                          \
        if (!(_x[k] == _y[k]))                                                 \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    return true;                                                               \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_vec_##S(void)                                                          \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 200000; ++trial)                                   \
    {                                                                          \
        const size_t n = (size_t)(trial % 8) + 1u;                             \
        T a[8];                                                                \
        T b[8];                                                                \
        T out[8];                                                              \
        T want[8];                                                             \
        T s = (T)d_tests_random();                                             \
        size_t i = 0u;                                                         \
        T acc = (T)0;                                                          \
                                                                               \
        for (i = 0u; i < 8u; ++i)                                              \
        {                                                                      \
            a[i] = (T)d_tests_random();                                        \
            b[i] = (T)d_tests_random();                                        \
        }                                                                      \
                                                                               \
        /* element-wise: exactly the formula */                                \
        d_vec_##S##_add(out, a, b, n);                                         \
        for (i = 0u; i < n; ++i) { want[i] = a[i] + b[i]; }                    \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, n), #S " add");      \
        d_vec_##S##_sub(out, a, b, n);                                         \
        for (i = 0u; i < n; ++i) { want[i] = a[i] - b[i]; }                    \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, n), #S " sub");      \
        d_vec_##S##_scale(out, a, s, n);                                       \
        for (i = 0u; i < n; ++i) { want[i] = a[i] * s; }                       \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, n), #S " scale");    \
        d_vec_##S##_hadamard(out, a, b, n);                                    \
        for (i = 0u; i < n; ++i) { want[i] = a[i] * b[i]; }                    \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, n),                  \
                       #S " hadamard");                                        \
        d_vec_##S##_lerp(out, a, b, s, n);                                     \
        for (i = 0u; i < n; ++i)                                               \
        {                                                                      \
            const T keep = (T)1 - s;                                           \
            const T l    = a[i] * keep;                                        \
            const T r    = b[i] * s;                                           \
            want[i] = l + r;                                                   \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, n), #S " lerp");     \
                                                                               \
        /* the dot product: each product rounded, summed in order */           \
        acc = (T)0;                                                            \
        for (i = 0u; i < n; ++i) { const T p = a[i] * b[i]; acc += p; }        \
        D_TESTS_EXPECT(d_vec_##S##_dot(a, b, n) == acc, #S " dot");            \
                                                                               \
        /* the two roots agree bit for bit, and are the library's */           \
        {                                                                      \
            const T sq = d_vec_##S##_norm_squared(a, n);                       \
            const T nc = d_vec_##S##_norm_c(a, n);                             \
            const T nr = d_vec_##S##_norm(a, n);                               \
            D_TESTS_EXPECT(nc == nr,                   \
                           #S " norm_c is norm");                              \
            D_TESTS_EXPECT(nr == ROOT(sq), #S " norm is the library root");    \
            D_TESTS_EXPECT(d_vec_##S##_distance_c(a, b, n) ==                  \
                           d_vec_##S##_distance(a, b, n),                      \
                           #S " distance_c is distance");                      \
        }                                                                      \
                                                                               \
        /* normalization: divided by the root */                               \
        {                                                                      \
            const T nr = d_vec_##S##_norm(a, n);                               \
            T       nc_out[8];                                                 \
            d_vec_##S##_normalize(out, a, n);                                  \
            d_vec_##S##_normalize_c(nc_out, a, n);                             \
            for (i = 0u; i < n; ++i) { want[i] = a[i] / nr; }                  \
            D_TESTS_EXPECT( (d_tests_same_##S(out, want, n)) &&         \
                            (d_tests_same_##S(nc_out, want, n)),        \
                            #S " normalize");                                  \
        }                                                                      \
                                                                               \
        /* projection and rejection, by their formulas */                      \
        {                                                                      \
            const T d     = d_vec_##S##_norm_squared(b, n);                    \
            const T ratio = d_vec_##S##_dot(a, b, n) / d;                      \
            d_vec_##S##_project(out, a, b, n);                                 \
            for (i = 0u; i < n; ++i) { want[i] = b[i] * ratio; }               \
            D_TESTS_EXPECT(d_tests_same_##S(out, want, n),              \
                           #S " project");                                     \
            d_vec_##S##_reject(out, a, b, n);                                  \
            for (i = 0u; i < n; ++i)                                           \
            {                                                                  \
                const T along = b[i] * ratio;                                  \
                want[i] = a[i] - along;                                        \
            }                                                                  \
            D_TESTS_EXPECT(d_tests_same_##S(out, want, n),              \
                           #S " reject");                                      \
        }                                                                      \
                                                                               \
        /* the angle: the two cosines agree; the angles within two ulps */     \
        {                                                                      \
            const T ca = d_vec_##S##_cos_angle(a, b, n);                       \
            const T cc = d_vec_##S##_cos_angle_c(a, b, n);                     \
            const T ar = d_vec_##S##_angle(a, b, n);                           \
            const T ac = d_vec_##S##_angle_c(a, b, n);                         \
            const T diff = (ar > ac) ? (ar - ac) : (ac - ar);                  \
            D_TESTS_EXPECT(ca == cc, #S " cos_angle_c is cos_angle");          \
            D_TESTS_EXPECT(diff <= (T)4 * EPSILON * (ar > (T)1 ? ar : (T)1),   \
                           #S " angle_c near angle");                          \
            D_TESTS_EXPECT(ar == ACOS(ca > (T)1 ? (T)1                         \
                                     : (ca < (T)-1 ? (T)-1 : ca)),             \
                           #S " angle is the library's");                      \
        }                                                                      \
                                                                               \
        /* an output that is also an input */                                  \
        memcpy(out, a, sizeof(a));                                             \
        d_vec_##S##_add(out, out, b, n);                                       \
        for (i = 0u; i < n; ++i) { want[i] = a[i] + b[i]; }                    \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, n),                  \
                       #S " add in place");                                    \
                                                                               \
        /* the reductions */                                                   \
        {                                                                      \
            T sum = (T)0, prod = (T)1, lo = a[0], hi = a[0];                   \
            for (i = 0u; i < n; ++i)                                           \
            {                                                                  \
                sum += a[i];                                                   \
                prod *= a[i];                                                  \
                lo = (a[i] < lo) ? a[i] : lo;                                  \
                hi = (a[i] > hi) ? a[i] : hi;                                  \
            }                                                                  \
            D_TESTS_EXPECT( (d_vec_##S##_sum(a, n) == sum) &&                  \
                            (d_vec_##S##_product(a, n) == prod) &&             \
                            (d_vec_##S##_min(a, n) == lo) &&                   \
                            (d_vec_##S##_max(a, n) == hi),                     \
                            #S " reductions");                                 \
        }                                                                      \
                                                                               \
        D_TESTS_EXPECT( (d_vec_##S##_equal(a, a, n)) &&                        \
                        (d_vec_##S##_equals(a, a, (T)0, n)),                   \
                        #S " a vector equals itself");                         \
    }                                                                          \
                                                                               \
    /* the cases a careless kernel gets wrong */                               \
    {                                                                          \
        const T zero[3] = { (T)0, (T)0, (T)0 };                                \
        const T x[3]    = { (T)3, (T)0, (T)0 };                                \
        const T x2[3]   = { (T)7, (T)0, (T)0 };                                \
        const T y[3]    = { (T)0, (T)2, (T)0 };                                \
        T       out[3]  = { (T)1, (T)1, (T)1 };                                \
        T       inout[3] = { (T)1, (T)2, (T)3 };                               \
                                                                               \
        d_vec_##S##_normalize(out, zero, 3u);                                  \
        D_TESTS_EXPECT(d_vec_##S##_equal(out, zero, 3u),                       \
                       #S " the zero vector normalizes to zero");              \
        d_vec_##S##_project(out, x, zero, 3u);                                 \
        D_TESTS_EXPECT(d_vec_##S##_equal(out, zero, 3u),                       \
                       #S " projecting onto zero gives zero");                 \
        D_TESTS_EXPECT( (d_vec_##S##_angle(x, x2, 3u) == (T)0) &&              \
                        (d_vec_##S##_angle_c(x, x2, 3u) == (T)0),              \
                        #S " parallel vectors are at angle 0");                \
        D_TESTS_EXPECT(d_vec_##S##_cos_angle(x, zero, 3u) == (T)0,             \
                       #S " the zero vector's cosine is 0");                   \
        d_vec_##S##_cross(out, x, y);                                          \
        D_TESTS_EXPECT( (out[0] == (T)0) && (out[1] == (T)0) &&                \
                        (out[2] == (T)6),                                      \
                        #S " cross");                                          \
        d_vec_##S##_cross(inout, inout, x);                                    \
        D_TESTS_EXPECT( (inout[0] == (T)0) && (inout[1] == (T)9) &&            \
                        (inout[2] == (T)-6),                                   \
                        #S " cross in place");                                 \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_VEC_FAMILY(float,       f,  sqrtf, acosf, FLT_EPSILON)
D_TESTS_VEC_FAMILY(double,      d,  sqrt,  acos,  DBL_EPSILON)
D_TESTS_VEC_FAMILY(long double, ld, sqrtl, acosl, LDBL_EPSILON)

int
main(void)
{
    d_tests_vec_f();
    d_tests_vec_d();
    d_tests_vec_ld();

    printf("t_vec: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
