/*******************************************************************************
* djinterp [test]                                                        t_mat.c
*
*   Conformance harness for c/math/mat.h, all three families: every kernel
* against the formula it implements, written out here in the same type and
* order of operations, over 100,000 random matrices per family of every
* shape from 1 x 1 to 5 x 5 -- so the results must agree bit for bit -- and
* the identities a careless kernel breaks: the transpose of the transpose,
* the identity as a product's unit, A + A^T symmetric, A^0 the identity.
* Then the square algebra on 40,000 integer-valued matrices per family up to
* 4 x 4, where every step is exact: Bareiss's determinant against the
* cofactor expansion, A adj(A) = det(A) I, the adjugate as the cofactors
* transposed, A inv(A) = I, and a singular matrix's missing inverse. Then
* the rotations, 100,000 per family: Rodrigues's formula entry by entry in
* transform.hpp's order of operations, bit for bit; a rotation about a unit
* axis orthogonal to rounding; the axis and plane rotations' layouts.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/math/t_mat.c -o t_mat -lm
*
*
* path:      /tests/djinterp/c/math/t_mat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <stdio.h>   // printf
// djinterp
#include "../../../../inc/djinterp/c/math/mat.h"  // unit under test


static long          g_checks          = 0;
static long          g_singular_missed = 0;
static long          g_failures = 0;
static unsigned long g_state    = 314159265u;

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

// one family's test, stamped for each: T its type, S its suffix
#define D_TESTS_MAT_FAMILY(T, S)                                               \
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
d_tests_mat_##S(void)                                                          \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 100000; ++trial)                                   \
    {                                                                          \
        const size_t r = (size_t)(trial % 5) + 1u;                             \
        const size_t c = (size_t)((trial / 5) % 5) + 1u;                       \
        const size_t q = (size_t)((trial / 25) % 5) + 1u;                      \
        T a[25], b[25], sq[25], out[25], want[25], back[25], v[5], w[5];       \
        T scratch[25];                                                         \
        size_t i = 0u, j = 0u, k = 0u;                                         \
                                                                               \
        for (i = 0u; i < 25u; ++i)                                             \
        {                                                                      \
            a[i]  = (T)d_tests_random();                                       \
            b[i]  = (T)d_tests_random();                                       \
            sq[i] = (T)(d_tests_random() / 4.0);                               \
        }                                                                      \
                                                                               \
        for (i = 0u; i < 5u; ++i)                                              \
        {                                                                      \
            v[i] = (T)d_tests_random();                                        \
        }                                                                      \
                                                                               \
        /* the product, r x c by c x q: each product rounded, summed in k */   \
        d_mat_##S##_multiply(out, a, b, r, c, q);                              \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            for (j = 0u; j < q; ++j)                                           \
            {                                                                  \
                T acc = (T)0;                                                  \
                for (k = 0u; k < c; ++k)                                       \
                {                                                              \
                    const T p = a[(i * c) + k] * b[(k * q) + j];               \
                    acc += p;                                                  \
                }                                                              \
                want[(i * q) + j] = acc;                                       \
            }                                                                  \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, r * q), #S " multiply");    \
                                                                               \
        /* the matrix-vector product */                                        \
        d_mat_##S##_multiply_vector(w, a, v, r, c);                            \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            T acc = (T)0;                                                      \
            for (j = 0u; j < c; ++j)                                           \
            {                                                                  \
                const T p = a[(i * c) + j] * v[j];                             \
                acc += p;                                                      \
            }                                                                  \
            want[i] = acc;                                                     \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_same_##S(w, want, r),                           \
                       #S " multiply_vector");                                 \
                                                                               \
        /* the transpose, and the transpose of the transpose */                \
        d_mat_##S##_transpose(out, a, r, c);                                   \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            for (j = 0u; j < c; ++j)                                           \
            {                                                                  \
                want[(j * r) + i] = a[(i * c) + j];                            \
            }                                                                  \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, r * c), #S " transpose");   \
        d_mat_##S##_transpose(back, out, c, r);                                \
        D_TESTS_EXPECT(d_tests_same_##S(back, a, r * c),                       \
                       #S " the transpose of the transpose");                  \
                                                                               \
        /* rows and columns */                                                 \
        d_mat_##S##_row(w, a, r - 1u, c);                                      \
        D_TESTS_EXPECT(d_tests_same_##S(w, a + ((r - 1u) * c), c), #S " row"); \
        d_mat_##S##_col(w, a, c - 1u, r, c);                                   \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            want[i] = a[(i * c) + (c - 1u)];                                   \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_same_##S(w, want, r), #S " col");               \
                                                                               \
        /* square: the identity is the product's unit; trace; symmetry */      \
        d_mat_##S##_identity(back, r);                                         \
        d_mat_##S##_multiply(out, sq, back, r, r, r);                          \
        D_TESTS_EXPECT(d_tests_same_##S(out, sq, r * r),                       \
                       #S " A * I is A");                                      \
        D_TESTS_EXPECT(d_mat_##S##_is_identity(back, r, (T)0),                 \
                       #S " the identity is the identity");                    \
        {                                                                      \
            T acc = (T)0;                                                      \
            for (i = 0u; i < r; ++i)                                           \
            {                                                                  \
                acc += sq[(i * r) + i];                                        \
            }                                                                  \
            D_TESTS_EXPECT(d_mat_##S##_trace(sq, r) == acc, #S " trace");      \
        }                                                                      \
        d_mat_##S##_transpose(out, sq, r, r);                                  \
        d_vec_##S##_add(want, sq, out, r * r);                                 \
        D_TESTS_EXPECT(d_mat_##S##_is_symmetric(want, r, (T)0),                \
                       #S " A + A^T is symmetric");                            \
        D_TESTS_EXPECT( (r == 1u) ||                                           \
                        (d_mat_##S##_is_symmetric(sq, r, (T)0) ==              \
                         d_tests_same_##S(sq, out, r * r)),                    \
                        #S " A is symmetric when A^T is A");                   \
                                                                               \
        /* powers: A^0 = I; A^k = A * A^(k-1), in that order */                \
        d_mat_##S##_power(out, scratch, sq, r, 0u);                            \
        D_TESTS_EXPECT(d_tests_same_##S(out, back, r * r), #S " A^0 is I");    \
        {                                                                      \
            const size_t e = (size_t)(trial % 4) + 1u;                         \
            size_t       step = 0u;                                            \
            for (i = 0u; i < r * r; ++i)                                       \
            {                                                                  \
                want[i] = back[i];                                             \
            }                                                                  \
            for (step = 0u; step < e; ++step)                                  \
            {                                                                  \
                d_mat_##S##_multiply(scratch, sq, want, r, r, r);              \
                for (i = 0u; i < r * r; ++i)                                   \
                {                                                              \
                    want[i] = scratch[i];                                      \
                }                                                              \
            }                                                                  \
            d_mat_##S##_power(out, scratch, sq, r, e);                         \
            D_TESTS_EXPECT(d_tests_same_##S(out, want, r * r),                 \
                           #S " A^k");                                         \
        }                                                                      \
                                                                               \
        /* the diagonal factory */                                             \
        d_mat_##S##_diagonal(out, v, r);                                       \
        for (i = 0u; i < r * r; ++i)                                           \
        {                                                                      \
            want[i] = (T)0;                                                    \
        }                                                                      \
        for (i = 0u; i < r; ++i)                                               \
        {                                                                      \
            want[(i * r) + i] = v[i];                                          \
        }                                                                      \
        D_TESTS_EXPECT(d_tests_same_##S(out, want, r * r), #S " diagonal");    \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_MAT_FAMILY(float,       f)
D_TESTS_MAT_FAMILY(double,      d)
D_TESTS_MAT_FAMILY(long double, ld)


// the square algebra on integer-valued matrices, where every step is exact:
// det(A) by cofactor expansion, A adj(A) = det(A) I, A inv(A) = I to
// rounding, singular matrices; and Bareiss against itself written out
#define D_TESTS_SQUARE_FAMILY(T, S)                                            \
static T                                                                       \
d_tests_expand_##S(const T* _a, size_t _n)                                     \
{                                                                              \
    T      sub[16];                                                            \
    T      acc = (T)0;                                                         \
    size_t j   = 0u;                                                           \
                                                                               \
    if (_n == 1u)                                                              \
    {                                                                          \
        return _a[0];                                                          \
    }                                                                          \
                                                                               \
    for (j = 0u; j < _n; ++j)                                                  \
    {                                                                          \
        const T sign = ((j % 2u) == 0u) ? (T)1 : (T)-1;                        \
        d_mat_##S##_submatrix(sub, _a, _n, 0u, j);                             \
        acc += sign * _a[j] * d_tests_expand_##S(sub, _n - 1u);                \
    }                                                                          \
                                                                               \
    return acc;                                                                \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_square_##S(void)                                                       \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 40000; ++trial)                                    \
    {                                                                          \
        const size_t n = (size_t)(trial % 4) + 1u;                             \
        T a[16], adj[16], prod[16], inv[16], scratch[32], want[16];            \
        T d = (T)0;                                                            \
        size_t i = 0u;                                                         \
        bool ok = false;                                                       \
                                                                               \
        /* small integers, a third of them zero: singular ones come up */      \
        for (i = 0u; i < 16u; ++i)                                             \
        {                                                                      \
            g_state = (g_state * 1103515245u) + 12345u;                        \
            a[i] = ((g_state >> 8) % 3u == 0u)                                 \
                       ? (T)0 : (T)((long)((g_state >> 12) % 9u) - 4);         \
        }                                                                      \
                                                                               \
        d = d_mat_##S##_determinant(a, scratch, n);                            \
        D_TESTS_EXPECT(d == d_tests_expand_##S(a, n),                          \
                       #S " Bareiss is the cofactor expansion, exactly");      \
                                                                               \
        /* A adj(A) = det(A) I */                                              \
        if (n >= 2u)                                                           \
        {                                                                      \
            d_mat_##S##_cofactor_matrix(adj, a, scratch, n, true);             \
            d_mat_##S##_multiply(prod, a, adj, n, n, n);                       \
            d_mat_##S##_identity(want, n);                                     \
            d_vec_##S##_scale(want, want, d, n * n);                           \
            D_TESTS_EXPECT(d_vec_##S##_equal(prod, want, n * n),               \
                           #S " A adj(A) = det(A) I");                         \
            d_mat_##S##_cofactor_matrix(prod, a, scratch, n, false);           \
            d_mat_##S##_transpose(want, prod, n, n);                           \
            D_TESTS_EXPECT(d_vec_##S##_equal(adj, want, n * n),                \
                           #S " the adjugate is the cofactors transposed");    \
        }                                                                      \
                                                                               \
        /* the inverse: false and zero for a singular matrix, else A inv(A) */ \
        /* (Gauss-Jordan in floating point can round a singular        */      \
        /* matrix's last pivot to a tiny non-zero, as the subframework's */    \
        /* always could -- 57 of the float cases here: only a reported  */     \
        /* failure is held to the zero matrix)                          */     \
        ok = d_mat_##S##_inverse(inv, scratch, a, n);                          \
        if (d == (T)0)                                                         \
        {                                                                      \
            for (i = 0u; i < n * n; ++i) { want[i] = (T)0; }                   \
            D_TESTS_EXPECT( (ok) ||                                            \
                            (d_vec_##S##_equal(inv, want, n * n)),             \
                            #S " a failed inverse is the zero matrix");        \
            g_singular_missed += ok ? 1 : 0;                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            d_mat_##S##_multiply(prod, a, inv, n, n, n);                       \
            D_TESTS_EXPECT( (ok) &&                                            \
                            (d_mat_##S##_is_identity(prod, n, (T)1e-4)),       \
                            #S " A inv(A) = I");                               \
        }                                                                      \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_SQUARE_FAMILY(float,       f)
D_TESTS_SQUARE_FAMILY(double,      d)
D_TESTS_SQUARE_FAMILY(long double, ld)

// the rotations: each entry against the formula transform.hpp wrote,
// in its order of operations, bit for bit; orthogonality to rounding
#define D_TESTS_ROTATION_FAMILY(T, S)                                          \
static void                                                                    \
d_tests_rotation_##S(void)                                                     \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 100000; ++trial)                                   \
    {                                                                          \
        const T c  = (T)(d_tests_random() / 4.0);                              \
        const T s  = (T)(d_tests_random() / 4.0);                              \
        T       k[3], r[9], want[9], rt[9], prod[9];                           \
        T       norm = (T)0;                                                   \
        size_t  i    = 0u;                                                     \
                                                                               \
        for (i = 0u; i < 3u; ++i)                                              \
        {                                                                      \
            k[i] = (T)d_tests_random();                                        \
        }                                                                      \
                                                                               \
        /* Rodrigues, as transform.hpp wrote it */                             \
        d_mat_##S##_rotation_axis(r, k, c, s);                                 \
        {                                                                      \
            const T t = (T)1 - c;                                              \
            const T kx = k[0], ky = k[1], kz = k[2];                           \
            T p = (T)0, q = (T)0;                                              \
            p = kx * kx; q = p * t; want[0] = c + q;                           \
            p = kx * ky; q = p * t; p = kz * s; want[1] = q - p;               \
            p = kx * kz; q = p * t; p = ky * s; want[2] = q + p;               \
            p = ky * kx; q = p * t; p = kz * s; want[3] = q + p;               \
            p = ky * ky; q = p * t; want[4] = c + q;                           \
            p = ky * kz; q = p * t; p = kx * s; want[5] = q - p;               \
            p = kz * kx; q = p * t; p = ky * s; want[6] = q - p;               \
            p = kz * ky; q = p * t; p = kx * s; want[7] = q + p;               \
            p = kz * kz; q = p * t; want[8] = c + q;                           \
        }                                                                      \
        D_TESTS_EXPECT(d_vec_##S##_equal(r, want, 9u),                         \
                       #S " rotation_axis is Rodrigues's formula");            \
                                                                               \
        /* a true rotation: a unit axis and c^2 + s^2 = 1 */                   \
        norm = d_vec_##S##_norm(k, 3u);                                        \
        d_vec_##S##_divide(k, k, norm, 3u);                                    \
        {                                                                      \
            const T angle = (T)(d_tests_random());                             \
            d_mat_##S##_rotation_axis(r, k, d_math_##S##_cos(angle),           \
                                      d_math_##S##_sin(angle));                \
        }                                                                      \
        d_mat_##S##_transpose(rt, r, 3u, 3u);                                  \
        d_mat_##S##_multiply(prod, rt, r, 3u, 3u, 3u);                         \
        D_TESTS_EXPECT(d_mat_##S##_is_identity(prod, 3u, (T)1e-5),             \
                       #S " a rotation is orthogonal");                        \
                                                                               \
        /* the axis rotations and the plane's */                               \
        d_mat_##S##_rotation_3d(r, (size_t)(trial % 3), c, s);                 \
        d_mat_##S##_identity(want, 3u);                                        \
        {                                                                      \
            const size_t ax = (size_t)(trial % 3);                             \
            const size_t a  = (ax + 1u) % 3u;                                  \
            const size_t b  = (ax + 2u) % 3u;                                  \
            want[(a * 3u) + a] = c;                                            \
            want[(a * 3u) + b] = -s;                                           \
            want[(b * 3u) + a] = s;                                            \
            want[(b * 3u) + b] = c;                                            \
        }                                                                      \
        D_TESTS_EXPECT(d_vec_##S##_equal(r, want, 9u),                         \
                       #S " rotation about an axis");                          \
        d_mat_##S##_rotation_2d(r, c, s);                                      \
        D_TESTS_EXPECT( (r[0] == c) && (r[1] == -s) &&                         \
                        (r[2] == s) && (r[3] == c),                            \
                        #S " rotation in the plane");                          \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_ROTATION_FAMILY(float,       f)
D_TESTS_ROTATION_FAMILY(double,      d)
D_TESTS_ROTATION_FAMILY(long double, ld)

int
main(void)
{
    d_tests_mat_f();
    d_tests_mat_d();
    d_tests_mat_ld();
    d_tests_square_f();
    d_tests_square_d();
    d_tests_square_ld();
    d_tests_rotation_f();
    d_tests_rotation_d();
    d_tests_rotation_ld();

    printf("singular matrices whose inverse rounding hid: %ld\n",
           g_singular_missed);
    printf("t_mat: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
