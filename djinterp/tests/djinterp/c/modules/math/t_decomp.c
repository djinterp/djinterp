/*******************************************************************************
* djinterp [test]                                                     t_decomp.c
*
*   Conformance harness for c/math/decomp.h, all three families: LU, its
* solve and determinant, triangular substitution and Q^T b against the
* decomposition and solve headers' loops written out here in the same
* order with every step stored, bit for bit, on random matrices of orders 1
* to 6; Householder QR, Cholesky, the one-sided Jacobi SVD and the cyclic
* Jacobi symmetric eigensolver, their _c forms against their library forms
* (the root is correctly rounded in both, so bit for bit too); and the
* algebra the factors must satisfy -- P A = L U, A = Q R with Q orthogonal
* and R upper triangular, A = L L^T, A = U S V^T with V orthogonal and S
* descending, A V = V L with V orthogonal and L ascending, A x = b -- to
* rounding; singular and non-positive-definite matrices detected.
*   The general eigenvalue core (chunk 13), its _c forms against the
* library's bit for bit: the Hessenberg form zero below the subdiagonal,
* its trace and norm kept; a symmetric matrix's eigenvalues Jacobi's to
* rounding, ascending; L T L^-1's, T's integer diagonal; 2^e A's, 2^e
* times A's exactly; an eigenvector unit, its largest entry positive, A x
* = l x to rounding; and matrices with known answers -- zero, identity,
* diagonal, triangular, a rotation and a cyclic shift (complex, flagged),
* a NaN and an infinity, no steps allowed.
*   The eigenvalue path of chunk 38, LAPACK's dgeev, at every matrix of
* math_tests_eig_refs.h -- each exact in float, its eigenvalues computed by
* mpmath at 80 digits with their condition numbers: the driver and its _c
* form bit for bit; each eigenvalue within 32 epsilons times |A|_F times its
* condition number of the reference, ascending, a pair exactly conjugate, a
* real one's imaginary part +0; each eigenvector unit, its largest entry
* real and positive, its residual within 32 epsilons of the balanced
* matrix's norm; 2^k A's results 2^k times A's, bit for bit; the old
* eigenvalues the driver's real parts; mat.h's solution of [A | B] LU's
* bits. Then each part alone: balancing exact, the Hessenberg form and the
* Schur form similarities to 8 n epsilons with Q and Z orthogonal, the
* Schur form quasi-triangular with standard blocks, the Schur vectors
* eigenvectors of it, each part's two forms alike; matrices with known
* answers at the range's ends; and the Jacobi tangent past 2^(EMAX/2),
* sym_eigen's and the SVD's. The largest measures are printed. From C++14,
* the driver of a 3-by-3 matrix with a complex pair, and sym_eigen at a
* threshold of 0, as constant expressions with the run time's bits.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_decomp.c -o t_decomp -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_decomp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.30
*******************************************************************************/
// std
#include <float.h>   // FLT_EPSILON, DBL_EPSILON, LDBL_EPSILON, FLT_MIN, ...
#include <math.h>    // HUGE_VAL
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/decimal.h"  // d_math_decimal_to_ld
#include "../../../../../inc/djinterp/c/modules/math/decomp.h"   // unit under test
#include "../../../../../inc/djinterp/c/modules/math/mat.h"      // d_mat_*_solve
#include "./math_tests_eig_refs.h"                    // the references


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 2236067u;

static
void d_tests_expect(
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

static
double d_tests_random(void)
{
    g_state = (g_state * 1103515245u) + 12345u;

    return ((double)((g_state >> 4) % 2000003u) / 250000.0) - 4.0;
}

// d_tests_point
//   m 2^e, exactly, as the references give an entry.
static
long double d_tests_point(
    long _m,
    int  _e
)
{
    return (long double)_m * d_internal_math_ld_pow2(_e);
}

// d_tests_text
//   the decimal _text, a reference's, rounded to long double by decimal.h.
static
long double d_tests_text(
    const char* _text
)
{
    long double value = 0.0L;

    (void)d_math_decimal_to_ld(&value, _text, strlen(_text));

    return value;
}

// one family's test: T its type, S its suffix, E a tolerance for what holds
// to rounding
#define D_TESTS_DECOMP_FAMILY(T, S, E)                                   \
static T                                                                       \
d_tests_abs_##S(T _x)                                                          \
{                                                                              \
    return (_x < (T)0) ? -_x : _x;                                             \
}                                                                              \
                                                                               \
/* the header's LU, written out with every step stored */                      \
static bool                                                                    \
d_tests_lu_##S(T* lu, size_t n, size_t* perm, T* sign)                         \
{                                                                              \
    bool singular = false;                                                     \
    size_t i, col, r, c;                                                       \
    *sign = (T)1;                                                              \
    for (i = 0; i < n; ++i) perm[i] = i;                                       \
    for (col = 0; col < n; ++col)                                              \
    {                                                                          \
        size_t pivot = col;                                                    \
        T maxv = d_tests_abs_##S(lu[col * n + col]);                           \
        for (r = col + 1; r < n; ++r)                                          \
        {                                                                      \
            const T v = d_tests_abs_##S(lu[r * n + col]);                      \
            if (v > maxv) { maxv = v; pivot = r; }                             \
        }                                                                      \
        if (lu[pivot * n + col] == (T)0) { singular = true; continue; }        \
        if (pivot != col)                                                      \
        {                                                                      \
            for (c = 0; c < n; ++c)                                            \
            {                                                                  \
                const T t = lu[col * n + c];                                   \
                lu[col * n + c] = lu[pivot * n + c];                           \
                lu[pivot * n + c] = t;                                         \
            }                                                                  \
            { const size_t t = perm[col]; perm[col] = perm[pivot];             \
              perm[pivot] = t; }                                               \
            *sign = -*sign;                                                    \
        }                                                                      \
        for (r = col + 1; r < n; ++r)                                          \
        {                                                                      \
            volatile T f = lu[r * n + col] / lu[col * n + col];                \
            lu[r * n + col] = f;                                               \
            for (c = col + 1; c < n; ++c)                                      \
            {                                                                  \
                volatile T prod = f * lu[col * n + c];                         \
                lu[r * n + c] = lu[r * n + c] - prod;                          \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    return singular;                                                           \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_decomp_##S(void)                                                       \
{                                                                              \
    long trial = 0;                                                            \
                                                                               \
    for (trial = 0; trial < 20000; ++trial)                                    \
    {                                                                          \
        const size_t n = (size_t)(1 + (trial % 6));                            \
        T a[36], lu[36], ref[36], b[6], x[6];                                  \
        size_t perm[6], rperm[6], i, j, k;                                     \
        T sign = (T)0, rsign = (T)0;                                           \
        bool sing, rsing, same = true;                                         \
                                                                               \
        for (i = 0; i < n * n; ++i) a[i] = (T)d_tests_random();                \
        for (i = 0; i < n; ++i) b[i] = (T)d_tests_random();                    \
        /* a singular one now and then: two equal rows */                      \
        if ((trial % 37) == 0 && n > 1)                                        \
        {                                                                      \
            for (j = 0; j < n; ++j) a[n + j] = a[j];                           \
        }                                                                      \
                                                                               \
        /* LU: the header's algorithm, bit for bit */                          \
        memcpy(lu, a, sizeof(T) * n * n);                                      \
        memcpy(ref, a, sizeof(T) * n * n);                                     \
        sing  = d_decomp_##S##_lu(lu, n, perm, &sign);                         \
        rsing = d_tests_lu_##S(ref, n, rperm, &rsign);                         \
        for (i = 0; i < n * n; ++i) same = same && (lu[i] == ref[i]);          \
        for (i = 0; i < n; ++i) same = same && (perm[i] == rperm[i]);          \
        D_TESTS_EXPECT(same && (sing == rsing) && (sign == rsign),             \
                       #S " lu is the header's algorithm");                    \
        if ((trial % 37) == 0 && n > 1)                                        \
        {                                                                      \
            D_TESTS_EXPECT(sing && (d_decomp_##S##_lu_det(lu, n, sign) ==      \
                                    (T)0),                                     \
                           #S " a singular matrix: detected, determinant 0");  \
        }                                                                      \
        if (sing) continue;                                                    \
                                                                               \
        /* A x = b, to rounding (a scale-aware bound) */                       \
        d_decomp_##S##_lu_solve(lu, perm, n, b, x);                            \
        {                                                                      \
            T worst = (T)0, scale = (T)1;                                      \
            for (i = 0; i < n; ++i)                                            \
            {                                                                  \
                T s = (T)0;                                                    \
                for (j = 0; j < n; ++j)                                        \
                {                                                              \
                    s += a[i * n + j] * x[j];                                  \
                    scale += d_tests_abs_##S(a[i * n + j] * x[j]);             \
                }                                                              \
                if (d_tests_abs_##S(s - b[i]) > worst)                         \
                    worst = d_tests_abs_##S(s - b[i]);                         \
            }                                                                  \
            /* ill-conditioned draws are allowed their growth */               \
            D_TESTS_EXPECT( (worst <= (T)E * (T)1e6 * scale) ||                \
                            (d_tests_abs_##S(d_decomp_##S##_lu_det(lu, n,      \
                                 sign)) < (T)1e-3),                            \
                            #S " lu_solve: A x = b");                          \
        }                                                                      \
                                                                               \
        /* triangular substitution: solve.hpp's loops, bit for bit */          \
        {                                                                      \
            T fx[6], bx[6], rf[6], rb[6];                                      \
            bool ok = true;                                                    \
            d_decomp_##S##_forward_sub(a, n, b, fx);                           \
            d_decomp_##S##_back_sub(a, n, b, bx);                              \
            for (i = 0; i < n; ++i)                                            \
            {                                                                  \
                T s = b[i];                                                    \
                for (k = 0; k < i; ++k)                                        \
                {                                                              \
                    volatile T prod = a[i * n + k] * rf[k];                    \
                    s = s - prod;                                              \
                }                                                              \
                rf[i] = s / a[i * n + i];                                      \
            }                                                                  \
            for (i = n; i-- > 0; )                                             \
            {                                                                  \
                T s = b[i];                                                    \
                for (k = i + 1; k < n; ++k)                                    \
                {                                                              \
                    volatile T prod = a[i * n + k] * rb[k];                    \
                    s = s - prod;                                              \
                }                                                              \
                rb[i] = s / a[i * n + i];                                      \
            }                                                                  \
            for (i = 0; i < n; ++i)                                            \
            {                                                                  \
                ok = ok && ((fx[i] == rf[i]) ||                                \
                            ((fx[i] != fx[i]) && (rf[i] != rf[i])));           \
                ok = ok && ((bx[i] == rb[i]) ||                                \
                            ((bx[i] != bx[i]) && (rb[i] != rb[i])));           \
            }                                                                  \
            D_TESTS_EXPECT(ok, #S " forward_sub and back_sub: the loops");     \
        }                                                                      \
                                                                               \
        /* QR of A (n by n) and of a taller block: the forms agree, and */     \
        /* Q R = A, Q^T Q = I, R upper                                  */     \
        {                                                                      \
            const size_t m = n + (size_t)(trial % 3);                          \
            T q[64], r[48], qc[64], rc[48], v[8], aa[48];                      \
            bool ok = true;                                                    \
            for (i = 0; i < m * n; ++i) aa[i] = (T)d_tests_random();           \
            for (i = 0; i < m * m; ++i) q[i] = qc[i] = (T)0;                   \
            for (i = 0; i < m; ++i) q[i * m + i] = qc[i * m + i] = (T)1;       \
            memcpy(r, aa, sizeof(T) * m * n);                                  \
            memcpy(rc, aa, sizeof(T) * m * n);                                 \
            d_decomp_##S##_qr(q, r, m, n, v);                                  \
            d_decomp_##S##_qr_c(qc, rc, m, n, v);                              \
            for (i = 0; i < m * m; ++i) ok = ok && (q[i] == qc[i]);            \
            for (i = 0; i < m * n; ++i) ok = ok && (r[i] == rc[i]);            \
            D_TESTS_EXPECT(ok, #S " qr_c is qr, bit for bit");                 \
            ok = true;                                                         \
            for (i = 0; i < m; ++i)                                            \
                for (j = 0; j < n; ++j)                                        \
                {                                                              \
                    T s = (T)0;                                                \
                    for (k = 0; k < m; ++k) s += q[i * m + k] * r[k * n + j];  \
                    ok = ok && (d_tests_abs_##S(s - aa[i * n + j]) <=          \
                                (T)E * (T)64);                                 \
                    if (i > j) ok = ok && (d_tests_abs_##S(r[i * n + j]) <=    \
                                           (T)E * (T)64);                      \
                }                                                              \
            for (i = 0; i < m; ++i)                                            \
                for (j = 0; j < m; ++j)                                        \
                {                                                              \
                    T s = (T)0;                                                \
                    for (k = 0; k < m; ++k) s += q[k * m + i] * q[k * m + j];  \
                    ok = ok && (d_tests_abs_##S(s - ((i == j) ? (T)1 : (T)0))  \
                                <= (T)E * (T)16);                              \
                }                                                              \
            D_TESTS_EXPECT(ok, #S " qr: Q R = A, Q orthogonal, R upper");      \
            /* Q^T b: the columns against b, in order */                       \
            {                                                                  \
                T bb[8], c[8];                                                 \
                for (i = 0; i < m; ++i) bb[i] = (T)d_tests_random();           \
                d_decomp_##S##_qt_mul(q, m, n, bb, c);                         \
                ok = true;                                                     \
                for (i = 0; i < n; ++i)                                        \
                {                                                              \
                    T s = (T)0;                                                \
                    for (k = 0; k < m; ++k)                                    \
                    {                                                          \
                        volatile T prod = q[k * m + i] * bb[k];                \
                        s = s + prod;                                          \
                    }                                                          \
                    ok = ok && (c[i] == s);                                    \
                }                                                              \
                D_TESTS_EXPECT(ok, #S " qt_mul: Q^T b's first n entries");     \
            }                                                                  \
        }                                                                      \
                                                                               \
        /* the SVD of a taller block: the forms agree; U S V^T = A, V */       \
        /* orthogonal, the values descending and non-negative           */     \
        {                                                                      \
            const size_t m = n + (size_t)(trial % 3);                          \
            T w[48], wc[48], aa[48], u[48], uc[48], sv[6], svc[6];             \
            T v[36], vc[36];                                                   \
            bool ok = true;                                                    \
            for (i = 0; i < m * n; ++i) aa[i] = w[i] = wc[i] =                 \
                                        (T)d_tests_random();                   \
            for (i = 0; i < n * n; ++i) v[i] = vc[i] = (T)0;                   \
            for (i = 0; i < n; ++i) v[i * n + i] = vc[i * n + i] = (T)1;       \
            d_decomp_##S##_svd(w, u, sv, v, m, n, (T)1e-12);                   \
            d_decomp_##S##_svd_c(wc, uc, svc, vc, m, n, (T)1e-12);             \
            for (i = 0; i < m * n; ++i) ok = ok && (u[i] == uc[i]);            \
            for (i = 0; i < n * n; ++i) ok = ok && (v[i] == vc[i]);            \
            for (i = 0; i < n; ++i) ok = ok && (sv[i] == svc[i]);              \
            D_TESTS_EXPECT(ok, #S " svd_c is svd, bit for bit");               \
            ok = true;                                                         \
            for (i = 0; i < n; ++i)                                            \
            {                                                                  \
                ok = ok && (sv[i] >= (T)0);                                    \
                if (i > 0) ok = ok && (sv[i] <= sv[i - 1]);                    \
            }                                                                  \
            for (i = 0; i < m; ++i)                                            \
                for (j = 0; j < n; ++j)                                        \
                {                                                              \
                    T s = (T)0;                                                \
                    for (k = 0; k < n; ++k)                                    \
                        s += u[i * n + k] * sv[k] * v[j * n + k];              \
                    ok = ok && (d_tests_abs_##S(s - aa[i * n + j]) <=          \
                                (T)E * (T)256);                                \
                }                                                              \
            for (i = 0; i < n; ++i)                                            \
                for (j = 0; j < n; ++j)                                        \
                {                                                              \
                    T s = (T)0;                                                \
                    for (k = 0; k < n; ++k) s += v[k * n + i] * v[k * n + j];  \
                    ok = ok && (d_tests_abs_##S(s - ((i == j) ? (T)1 : (T)0))  \
                                <= (T)E * (T)64);                              \
                }                                                              \
            D_TESTS_EXPECT(ok, #S " svd: U S V^T = A, V orthogonal, sorted"); \
        }                                                                      \
                                                                               \
        /* the symmetric eigensolver on (A + A^T) / 2: the forms agree; */    \
        /* A V = V L, V orthogonal, the values ascending                */    \
        {                                                                      \
            T sym[36], w[36], wc[36], val[6], valc[6], vec[36], vecc[36];      \
            bool ok = true;                                                    \
            for (i = 0; i < n; ++i)                                            \
                for (j = 0; j < n; ++j)                                        \
                    sym[i * n + j] = (a[i * n + j] + a[j * n + i]) / (T)2;     \
            for (i = 0; i < n * n; ++i) w[i] = wc[i] = sym[i];                 \
            for (i = 0; i < n * n; ++i) vec[i] = vecc[i] = (T)0;               \
            for (i = 0; i < n; ++i) vec[i * n + i] = vecc[i * n + i] = (T)1;   \
            d_decomp_##S##_sym_eigen(w, val, vec, n, (T)E * (T)8);             \
            d_decomp_##S##_sym_eigen_c(wc, valc, vecc, n, (T)E * (T)8);        \
            for (i = 0; i < n; ++i) ok = ok && (val[i] == valc[i]);            \
            for (i = 0; i < n * n; ++i) ok = ok && (vec[i] == vecc[i]);        \
            D_TESTS_EXPECT(ok, #S " sym_eigen_c is sym_eigen, bit for bit");   \
            ok = true;                                                         \
            for (i = 1; i < n; ++i) ok = ok && (val[i - 1] <= val[i]);         \
            for (i = 0; i < n; ++i)                                            \
                for (j = 0; j < n; ++j)                                        \
                {                                                              \
                    T av = (T)0, vv = (T)0;                                    \
                    for (k = 0; k < n; ++k)                                    \
                    {                                                          \
                        av += sym[i * n + k] * vec[k * n + j];                 \
                        vv += vec[k * n + i] * vec[k * n + j];                 \
                    }                                                          \
                    ok = ok && (d_tests_abs_##S(av - vec[i * n + j] * val[j])  \
                                <= (T)E * (T)512);                             \
                    ok = ok && (d_tests_abs_##S(vv - ((i == j) ? (T)1 : (T)0)) \
                                <= (T)E * (T)64);                              \
                }                                                              \
            D_TESTS_EXPECT(ok, #S " sym_eigen: A V = V L, V orthogonal");      \
        }                                                                      \
                                                                               \
        /* Cholesky of A A^T + n I: the forms agree, L L^T = A, the solve */   \
        {                                                                      \
            T spd[36], l[36], lc[36];                                          \
            bool ok = true, spd_ok, spd_okc;                                   \
            for (i = 0; i < n; ++i)                                            \
                for (j = 0; j < n; ++j)                                        \
                {                                                              \
                    T s = (i == j) ? (T)n : (T)0;                              \
                    for (k = 0; k < n; ++k) s += a[i * n + k] * a[j * n + k];  \
                    spd[i * n + j] = s;                                        \
                }                                                              \
            for (i = 0; i < n * n; ++i) l[i] = lc[i] = (T)0;                   \
            spd_ok  = d_decomp_##S##_cholesky(l, spd, n);                      \
            spd_okc = d_decomp_##S##_cholesky_c(lc, spd, n);                   \
            for (i = 0; i < n * n; ++i) ok = ok && (l[i] == lc[i]);            \
            D_TESTS_EXPECT(spd_ok && spd_okc && ok,                            \
                           #S " cholesky_c is cholesky; A A^T + n I is SPD");  \
            ok = true;                                                         \
            for (i = 0; i < n; ++i)                                            \
                for (j = 0; j <= i; ++j)                                       \
                {                                                              \
                    T s = (T)0;                                                \
                    for (k = 0; k < n; ++k) s += l[i * n + k] * l[j * n + k];  \
                    ok = ok && (d_tests_abs_##S(s - spd[i * n + j]) <=         \
                                (T)E * (T)64 * ((T)1 + spd[i * n + i]));       \
                }                                                              \
            d_decomp_##S##_cholesky_solve(l, n, b, x);                         \
            for (i = 0; i < n; ++i)                                            \
            {                                                                  \
                T s = (T)0;                                                    \
                for (j = 0; j < n; ++j) s += spd[i * n + j] * x[j];            \
                ok = ok && (d_tests_abs_##S(s - b[i]) <= (T)E * (T)4096);      \
            }                                                                  \
            D_TESTS_EXPECT(ok, #S " cholesky: L L^T = A, A x = b");            \
            /* a matrix with a negative diagonal is not SPD */                 \
            spd[0] = -(T)1;                                                    \
            for (i = 0; i < n * n; ++i) l[i] = (T)0;                           \
            D_TESTS_EXPECT(!d_decomp_##S##_cholesky(l, spd, n),                \
                           #S " a non-SPD matrix is detected");                \
        }                                                                      \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_DECOMP_FAMILY(float,       f,  1e-6)
D_TESTS_DECOMP_FAMILY(double,      d,  1e-15)
D_TESTS_DECOMP_FAMILY(long double, ld, 1e-15)

/* the general eigenvalue core, chunk 13: T its type, S its suffix, EPS */
/* its epsilon, SCALE_UP an exponent its range takes both ways, and     */
/* INTEGER_BOUND the known-spectrum check's bound, in epsilon ||A||_F   */
/* (chunk 13's record: integer_spectrum_c13.c measured at most 13.6     */
/* in 100,000)                                                          */
#define D_TESTS_EIGEN_FAMILY(T, S, EPS, SCALE_UP, INTEGER_BOUND)               \
/* the general eigenvalue core: every result against what it must */           \
/* satisfy, the _c forms against the library's, bit for bit         */         \
static bool                                                                    \
d_tests_same_##S(T _x, T _y)                                                   \
{                                                                              \
    return (_x == _y) || ((_x != _x) && (_y != _y));                           \
}                                                                              \
                                                                               \
static T                                                                       \
d_tests_fro_##S(const T* a, size_t n)                                          \
{                                                                              \
    T s = (T)0;                                                                \
    size_t i;                                                                  \
    for (i = 0; i < n * n; ++i) s += a[i] * a[i];                              \
    return d_math_##S##_sqrt(s);                                               \
}                                                                              \
                                                                               \
/* the eigenvalues of the n-by-n a, both forms; whether they agree */          \
static bool                                                                    \
d_tests_eigenvalues_##S(const T* a, size_t n, T* val, bool* conv,              \
                        size_t max_iter)                                       \
{                                                                              \
    T w[64], wc[64], valc[8];                                                  \
    bool convc, same;                                                          \
    size_t i;                                                                  \
    memcpy(w, a, sizeof(T) * n * n);                                           \
    memcpy(wc, a, sizeof(T) * n * n);                                          \
    *conv = d_decomp_##S##_eigenvalues(w, n, val, max_iter, (T)0);             \
    convc = d_decomp_##S##_eigenvalues_c(wc, n, valc, max_iter, (T)0);         \
    same  = (*conv == convc);                                                  \
    for (i = 0; i < n; ++i) same = same && d_tests_same_##S(val[i], valc[i]);  \
    return same;                                                               \
}                                                                              \
                                                                               \
/* an eigenvector of the n-by-n a for l, both forms; whether they agree */     \
static bool                                                                    \
d_tests_eigenvector_##S(const T* a, size_t n, T l, T* x, bool* found)          \
{                                                                              \
    T lu[64], xc[8], y[8];                                                     \
    size_t perm[8], i;                                                         \
    bool foundc, same;                                                         \
    *found = d_decomp_##S##_eigenvector(a, n, l, lu, perm, x, y);              \
    foundc = d_decomp_##S##_eigenvector_c(a, n, l, lu, perm, xc, y);           \
    same   = (*found == foundc);                                               \
    for (i = 0; (i < n) && *found; ++i)                                        \
        same = same && d_tests_same_##S(x[i], xc[i]);                          \
    return same;                                                               \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_eigen_##S(void)                                                        \
{                                                                              \
    const T eps = EPS;                                                         \
    long trial;                                                                \
    size_t i, j, k;                                                            \
                                                                               \
    for (trial = 0; trial < 3000; ++trial)                                     \
    {                                                                          \
        const size_t n = (size_t)(1 + (trial % 8));                            \
        T a[64], h[64], hc[64], sym[64], w[64], val[8], jv[8], jvec[64];       \
        T tr_a = (T)0, tr_h = (T)0, f2a = (T)0, f2h = (T)0, fro, fsym;         \
        bool ok = true, conv = false;                                          \
                                                                               \
        for (i = 0; i < n * n; ++i) a[i] = (T)d_tests_random();                \
        fro = d_tests_fro_##S(a, n);                                           \
                                                                               \
        /* the Hessenberg form: the forms agree; zero below the */             \
        /* subdiagonal; the trace and the Frobenius norm kept   */             \
        memcpy(h, a, sizeof(T) * n * n);                                       \
        memcpy(hc, a, sizeof(T) * n * n);                                      \
        d_decomp_##S##_hessenberg(h, n);                                       \
        d_decomp_##S##_hessenberg_c(hc, n);                                    \
        for (i = 0; i < n * n; ++i) ok = ok && (h[i] == hc[i]);                \
        D_TESTS_EXPECT(ok, #S " hessenberg_c is hessenberg, bit for bit");     \
        ok = true;                                                             \
        for (i = 0; i < n; ++i)                                                \
            for (j = 0; j + 1 < i; ++j) ok = ok && (h[i * n + j] == (T)0);     \
        for (i = 0; i < n; ++i)                                                \
        {                                                                      \
            tr_a += a[i * n + i];                                              \
            tr_h += h[i * n + i];                                              \
        }                                                                      \
        for (i = 0; i < n * n; ++i)                                            \
        {                                                                      \
            f2a += a[i] * a[i];                                                \
            f2h += h[i] * h[i];                                                \
        }                                                                      \
        ok = ok && (d_tests_abs_##S(tr_a - tr_h) <= (T)8 * (T)n * eps * fro);  \
        ok = ok && (d_tests_abs_##S(f2a - f2h) <= (T)16 * (T)n * eps * f2a);   \
        D_TESTS_EXPECT(ok, #S " hessenberg: zero below, trace and norm kept"); \
                                                                               \
        /* (A + A^T) / 2: real eigenvalues, ascending, Jacobi's to */          \
        /* rounding, both forms alike                              */          \
        for (i = 0; i < n; ++i)                                                \
            for (j = 0; j < n; ++j)                                            \
                sym[i * n + j] = (a[i * n + j] + a[j * n + i]) / (T)2;         \
        fsym = d_tests_fro_##S(sym, n);                                        \
        D_TESTS_EXPECT(d_tests_eigenvalues_##S(sym, n, val, &conv, 1000u),     \
                       #S " eigenvalues_c is eigenvalues, bit for bit");       \
        memcpy(w, sym, sizeof(T) * n * n);                                     \
        for (i = 0; i < n * n; ++i) jvec[i] = (T)0;                            \
        for (i = 0; i < n; ++i) jvec[i * n + i] = (T)1;                        \
        d_decomp_##S##_sym_eigen(w, jv, jvec, n, eps * fsym);                  \
        ok = conv;                                                             \
        for (i = 1; i < n; ++i) ok = ok && (val[i - 1] <= val[i]);             \
        for (i = 0; i < n; ++i)                                                \
            ok = ok && (d_tests_abs_##S(val[i] - jv[i]) <=                     \
                        (T)32 * eps * fsym);                                   \
        D_TESTS_EXPECT(ok, #S " eigenvalues, symmetric: Jacobi's, ascending"); \
                                                                               \
        /* an eigenvector for each: unit, its largest entry positive, */       \
        /* A x = l x to rounding, both forms alike                    */       \
        for (k = 0; k < n; ++k)                                                \
        {                                                                      \
            T x[8], norm2 = (T)0, top = (T)0, res = (T)0;                      \
            bool found = false;                                                \
            D_TESTS_EXPECT(d_tests_eigenvector_##S(sym, n, val[k], x, &found), \
                           #S " eigenvector_c is eigenvector, bit for bit");   \
            for (i = 0; (i < n) && found; ++i)                                 \
            {                                                                  \
                T s = (T)0;                                                    \
                norm2 += x[i] * x[i];                                          \
                if (d_tests_abs_##S(x[i]) > d_tests_abs_##S(top)) top = x[i];  \
                for (j = 0; j < n; ++j) s += sym[i * n + j] * x[j];            \
                s -= val[k] * x[i];                                            \
                if (d_tests_abs_##S(s) > res) res = d_tests_abs_##S(s);        \
            }                                                                  \
            ok = found &&                                                      \
                 (d_tests_abs_##S(norm2 - (T)1) <= (T)8 * (T)n * eps);         \
            ok = ok && (top > (T)0) && (res <= (T)64 * eps * fsym);            \
            D_TESTS_EXPECT(ok, #S " eigenvector: unit, A x = l x");            \
        }                                                                      \
                                                                               \
        /* A itself, and A scaled by 2^e: the eigenvalues scale */             \
        /* exactly, converged() stays, both forms alike         */             \
        {                                                                      \
            const int e = ((trial % 2) == 0) ? SCALE_UP : -SCALE_UP;           \
            const T p2 = d_internal_math_##S##_pow2(e);                        \
            T sa[64], v1[8], v2[8];                                            \
            bool c1 = false, c2 = false;                                       \
            ok = d_tests_eigenvalues_##S(a, n, v1, &c1, 1000u);                \
            for (i = 0; i < n * n; ++i) sa[i] = a[i] * p2;                     \
            ok = ok && d_tests_eigenvalues_##S(sa, n, v2, &c2, 1000u);         \
            ok = ok && (c1 == c2);                                             \
            for (i = 0; i < n; ++i)                                            \
                ok = ok && d_tests_same_##S(v1[i] * p2, v2[i]);                \
            D_TESTS_EXPECT(ok, #S " eigenvalues of 2^e A: 2^e times A's");     \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* a known integer spectrum, not symmetric: A = L T L^-1, L unit */        \
    /* lower with 0 and 1 below the diagonal, T upper with distinct  */        \
    /* integers on the diagonal and -1, 0, 1 above: A is integer,    */        \
    /* exact, and its eigenvalues are T's diagonal                   */        \
    for (trial = 0; trial < 3000; ++trial)                                     \
    {                                                                          \
        const size_t n = (size_t)(2 + (trial % 5));                            \
        T l[64], li[64], t[64], lt[64], a[64], val[8], want[8];                \
        T worst = (T)0, fro;                                                   \
        bool conv = false, ok;                                                 \
        for (i = 0; i < n * n; ++i) l[i] = li[i] = t[i] = (T)0;                \
        for (i = 0; i < n; ++i)                                                \
        {                                                                      \
            l[i * n + i] = (T)1;                                               \
            for (j = 0; j < i; ++j)                                            \
                l[i * n + j] = (d_tests_random() > (T)1) ? (T)1 : (T)0;        \
            for (j = i + 1; j < n; ++j)                                        \
                t[i * n + j] = (T)(int)(d_tests_random() / (T)2);              \
            t[i * n + i] = (T)(int)(((i * 5 + (size_t)trial) % 11)) - (T)5;    \
            want[i] = t[i * n + i];                                            \
        }                                                                      \
        /* L^-1 by forward substitution, exactly */                            \
        for (j = 0; j < n; ++j)                                                \
            for (i = 0; i < n; ++i)                                            \
            {                                                                  \
                T s = (i == j) ? (T)1 : (T)0;                                  \
                for (k = 0; k < i; ++k) s -= l[i * n + k] * li[k * n + j];     \
                li[i * n + j] = s;                                             \
            }                                                                  \
        for (i = 0; i < n; ++i)                                                \
            for (j = 0; j < n; ++j)                                            \
            {                                                                  \
                T s = (T)0;                                                    \
                for (k = 0; k < n; ++k) s += l[i * n + k] * t[k * n + j];      \
                lt[i * n + j] = s;                                             \
            }                                                                  \
        for (i = 0; i < n; ++i)                                                \
            for (j = 0; j < n; ++j)                                            \
            {                                                                  \
                T s = (T)0;                                                    \
                for (k = 0; k < n; ++k) s += lt[i * n + k] * li[k * n + j];    \
                a[i * n + j] = s;                                              \
            }                                                                  \
        fro = d_tests_fro_##S(a, n);                                           \
        /* the wanted spectrum, ascending */                                   \
        for (i = 0; i < n; ++i)                                                \
            for (j = i + 1; j < n; ++j)                                        \
                if (want[j] < want[i])                                         \
                {                                                              \
                    const T tmp = want[i];                                     \
                    want[i] = want[j];                                         \
                    want[j] = tmp;                                             \
                }                                                              \
        ok = d_tests_eigenvalues_##S(a, n, val, &conv, 1000u);                 \
        D_TESTS_EXPECT(ok, #S " eigenvalues_c is eigenvalues, bit for bit");   \
        for (i = 0; i < n; ++i)                                                \
            if (d_tests_abs_##S(val[i] - want[i]) > worst)                     \
                worst = d_tests_abs_##S(val[i] - want[i]);                     \
        D_TESTS_EXPECT(conv && (worst <= (T)INTEGER_BOUND * eps * fro),        \
                       #S " eigenvalues of L T L^-1: T's diagonal");           \
    }                                                                          \
                                                                               \
    /* matrices with known answers */                                          \
    {                                                                          \
        const T zero3[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };                      \
        const T id4[16]  = { 1, 0, 0, 0, 0, 1, 0, 0,                           \
                             0, 0, 1, 0, 0, 0, 0, 1 };                         \
        const T diag3[9] = { 3, 0, 0, 0, 1, 0, 0, 0, 2 };                      \
        const T up4[16]  = { 4, 1, 1, 1, 0, 3, 1, 1,                           \
                             0, 0, 2, 1, 0, 0, 0, 1 };                         \
        const T swap2[4] = { 0, 1, 1, 0 };                                     \
        const T rot2[4]  = { 0, -1, 1, 0 };                                    \
        const T cyc3[9]  = { 0, 1, 0, 0, 0, 1, 1, 0, 0 };                      \
        const T sym2[4]  = { 2, 1, 1, 2 };                                     \
        const T sym3[9]  = { 2, 1, 0, 1, 2, 1, 0, 1, 2 };                      \
        T bad[9], val[8], x[8];                                                \
        bool conv = false, found = false, ok;                                  \
                                                                               \
        ok = d_tests_eigenvalues_##S(zero3, 3, val, &conv, 1000u);             \
        D_TESTS_EXPECT(ok && conv && (val[0] == (T)0) && (val[2] == (T)0),     \
                       #S " eigenvalues: the zero matrix");                    \
        ok = d_tests_eigenvalues_##S(id4, 4, val, &conv, 1000u);               \
        D_TESTS_EXPECT(ok && conv && (val[0] == (T)1) && (val[3] == (T)1),     \
                       #S " eigenvalues: the identity");                       \
        ok = d_tests_eigenvalues_##S(diag3, 3, val, &conv, 1000u);             \
        D_TESTS_EXPECT(ok && conv && (val[0] == (T)1) && (val[1] == (T)2) &&   \
                       (val[2] == (T)3),                                       \
                       #S " eigenvalues: a diagonal, exactly, ascending");     \
        ok = d_tests_eigenvalues_##S(up4, 4, val, &conv, 1000u);               \
        D_TESTS_EXPECT(ok && conv && (val[0] == (T)1) && (val[1] == (T)2) &&   \
                       (val[2] == (T)3) && (val[3] == (T)4),                   \
                       #S " eigenvalues: upper triangular, exactly");          \
        ok = d_tests_eigenvalues_##S(swap2, 2, val, &conv, 1000u);             \
        D_TESTS_EXPECT(ok && conv && (val[0] == (T)-1) && (val[1] == (T)1),    \
                       #S " eigenvalues: [[0 1] [1 0]], exactly");             \
        ok = d_tests_eigenvalues_##S(rot2, 2, val, &conv, 1000u);              \
        D_TESTS_EXPECT(ok && !conv && (val[0] == (T)0) && (val[1] == (T)0),    \
                       #S " eigenvalues: a rotation: real parts, flagged");    \
        ok = d_tests_eigenvalues_##S(cyc3, 3, val, &conv, 1000u);              \
        D_TESTS_EXPECT(ok && !conv &&                                          \
                       (d_tests_abs_##S(val[0] + (T)0.5) <= (T)8 * eps) &&     \
                       (d_tests_abs_##S(val[1] + (T)0.5) <= (T)8 * eps) &&     \
                       (d_tests_abs_##S(val[2] - (T)1) <= (T)8 * eps),         \
                       #S " eigenvalues: a cyclic shift, the real one found"); \
        memcpy(bad, diag3, sizeof(bad));                                       \
        bad[5] = d_internal_math_##S##_nan();                                  \
        ok = d_tests_eigenvalues_##S(bad, 3, val, &conv, 1000u);               \
        D_TESTS_EXPECT(ok && !conv && (val[0] != val[0]) &&                    \
                       (val[2] != val[2]),                                     \
                       #S " eigenvalues: a NaN gives NaN, flagged");           \
        bad[5] = -(T)HUGE_VAL;                                                 \
        ok = d_tests_eigenvalues_##S(bad, 3, val, &conv, 1000u);               \
        D_TESTS_EXPECT(ok && !conv && (val[1] != val[1]),                      \
                       #S " eigenvalues: an infinity gives NaN, flagged");     \
        ok = d_tests_eigenvalues_##S(sym3, 3, val, &conv, 0u);                 \
        D_TESTS_EXPECT(ok && !conv,                                            \
                       #S " eigenvalues: no steps allowed, not converged");    \
                                                                               \
        /* eigenvectors with known answers */                                  \
        ok = d_tests_eigenvector_##S(diag3, 3, (T)2, x, &found);               \
        D_TESTS_EXPECT(ok && found && (x[2] == (T)1) &&                        \
                       (d_tests_abs_##S(x[0]) <= eps) &&                       \
                       (d_tests_abs_##S(x[1]) <= eps),                         \
                       #S " eigenvector: diag(3, 1, 2) for 2 is e3");          \
        ok = d_tests_eigenvector_##S(sym2, 2, (T)1, x, &found);                \
        D_TESTS_EXPECT(ok && found &&                                          \
                       (d_tests_abs_##S(x[0] - (T)0.70710678118654752440L) <=  \
                        (T)4 * eps) &&                                         \
                       (d_tests_abs_##S(x[1] + (T)0.70710678118654752440L) <=  \
                        (T)4 * eps),                                           \
                       #S " eigenvector: [[2 1] [1 2]] for 1, (1, -1)/r2");    \
        ok = d_tests_eigenvector_##S(zero3, 3, (T)0, x, &found);               \
        D_TESTS_EXPECT(ok && !found, #S " eigenvector: none for zero");        \
        bad[5] = d_internal_math_##S##_nan();                                  \
        ok = d_tests_eigenvector_##S(bad, 3, (T)1, x, &found);                 \
        D_TESTS_EXPECT(ok && !found, #S " eigenvector: none with a NaN");      \
        ok = d_tests_eigenvector_##S(diag3, 3, d_internal_math_##S##_nan(), x, \
                                     &found);                                  \
        D_TESTS_EXPECT(ok && !found, #S " eigenvector: none for a NaN value"); \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_EIGEN_FAMILY(float,       f,  FLT_EPSILON,  30,  64)
D_TESTS_EIGEN_FAMILY(double,      d,  DBL_EPSILON,  500, 64)
D_TESTS_EIGEN_FAMILY(long double, ld, LDBL_EPSILON, 500, 64)

/* the eigenvalue path of chunk 38, every case of math_tests_eig_refs.h:  */
/* the driver and its _c form, bit for bit; each eigenvalue within        */
/* VALUE_BOUND epsilons times |A|_F times its condition number of the     */
/* reference, ascending, a pair exactly conjugate, a real one's imaginary */
/* part +0; each eigenvector unit, its largest entry real and positive, a */
/* real eigenvalue's real, a pair's conjugate, its residual within        */
/* VECTOR_BOUND epsilons of the balanced matrix's norm, as LAPACK's       */
/* backward error is; 2^SHIFT A's results 2^SHIFT times A's, bit for bit; */
/* the old eigenvalues the driver's real parts. Then the parts: balancing */
/* exact, the isolated rows and columns triangular; the Hessenberg form   */
/* zero below the subdiagonal, Q orthogonal, Q H Q^T the balanced matrix; */
/* the Schur form quasi-triangular, its blocks of two standard, Z         */
/* orthogonal, Z T Z^T the balanced matrix, all to SCHUR_BOUND n epsilons;*/
/* the Schur vectors eigenvectors of it; each part's _c form bit for bit. */
#define D_TESTS_SCHUR_FAMILY(T, S, EPS, EMAX, SHIFT, BIG, SMALL,               \
                             VALUE_BOUND, VECTOR_BOUND, SCHUR_BOUND)           \
static long double g_worst_value_##S  = 0.0L;                                  \
static long double g_worst_vector_##S = 0.0L;                                  \
static long double g_worst_schur_##S  = 0.0L;                                  \
                                                                               \
static bool                                                                    \
d_tests_csame_##S(struct d_cplx_##S _x, struct d_cplx_##S _y)                  \
{                                                                              \
    return d_tests_same_##S(_x.part[0], _y.part[0]) &&                         \
           d_tests_same_##S(_x.part[1], _y.part[1]);                           \
}                                                                              \
                                                                               \
static bool                                                                    \
d_tests_plus_zero_##S(T _x)                                                    \
{                                                                              \
    return (_x == (T)0) && (((T)1 / _x) > (T)0);                               \
}                                                                              \
                                                                               \
/* the n-by-n _a's Frobenius norm, in long double */                           \
static long double                                                             \
d_tests_fro_ld_##S(const T* _a, size_t _n)                                     \
{                                                                              \
    long double s = 0.0L;                                                      \
    size_t i;                                                                  \
    for (i = 0; i < _n * _n; ++i)                                              \
        s += (long double)_a[i] * (long double)_a[i];                          \
    return d_math_ld_sqrt(s);                                                  \
}                                                                              \
                                                                               \
/* the largest of |X^T X - I|'s entries, in long double */                     \
static long double                                                             \
d_tests_orthogonality_##S(const T* _x, size_t _n)                              \
{                                                                              \
    long double worst = 0.0L;                                                  \
    size_t i, j, k;                                                            \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            long double s = (i == j) ? -1.0L : 0.0L;                           \
            for (k = 0; k < _n; ++k)                                           \
                s += (long double)_x[k * _n + i] *                             \
                     (long double)_x[k * _n + j];                              \
            if (s < 0.0L) s = -s;                                              \
            if (s > worst) worst = s;                                          \
        }                                                                      \
    return worst;                                                              \
}                                                                              \
                                                                               \
/* |X M X^T - B|_F, in long double */                                          \
static long double                                                             \
d_tests_similar_##S(const T* _x, const T* _m, const T* _b, size_t _n)          \
{                                                                              \
    long double xm[144], s2 = 0.0L;                                            \
    size_t i, j, k;                                                            \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            long double s = 0.0L;                                              \
            for (k = 0; k < _n; ++k)                                           \
                s += (long double)_x[i * _n + k] *                             \
                     (long double)_m[k * _n + j];                              \
            xm[i * _n + j] = s;                                                \
        }                                                                      \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            long double s = -(long double)_b[i * _n + j];                      \
            for (k = 0; k < _n; ++k)                                           \
                s += xm[i * _n + k] * (long double)_x[j * _n + k];             \
            s2 += s * s;                                                       \
        }                                                                      \
    return d_math_ld_sqrt(s2);                                                 \
}                                                                              \
                                                                               \
/* |A v - l v| / (eps |B|_F |v|) with each row q divided by _s[q], the */      \
/* balancing's factor of it: the residual of the balanced matrix B, of */      \
/* whose norm a backward-stable method's error is a few epsilons       */      \
static long double                                                             \
d_tests_residual_##S(const T* _a, size_t _n, const long double* _vr,           \
                     const long double* _vi, long double _lr,                  \
                     long double _li, const T* _s, long double _norm_b)        \
{                                                                              \
    long double r2 = 0.0L, v2 = 0.0L;                                          \
    size_t q, j;                                                               \
    for (q = 0; q < _n; ++q)                                                   \
    {                                                                          \
        long double rr = -(_lr * _vr[q] - _li * _vi[q]);                       \
        long double ri = -(_lr * _vi[q] + _li * _vr[q]);                       \
        const long double s = (long double)_s[q];                              \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            rr += (long double)_a[q * _n + j] * _vr[j];                        \
            ri += (long double)_a[q * _n + j] * _vi[j];                        \
        }                                                                      \
        r2 += (rr * rr + ri * ri) / (s * s);                                   \
        v2 += (_vr[q] * _vr[q] + _vi[q] * _vi[q]) / (s * s);                   \
    }                                                                          \
    if ((v2 == 0.0L) || (_norm_b == 0.0L)) return (r2 == 0.0L) ? 0.0L : 1e30L; \
    return d_math_ld_sqrt(r2) /                                                \
           ((long double)EPS * _norm_b * d_math_ld_sqrt(v2));                  \
}                                                                              \
                                                                               \
/* the case's matrix in the family, exactly */                                 \
static void                                                                    \
d_tests_case_##S(const struct dj_tests_eig_case* _c, T* _a)                     \
{                                                                              \
    int k;                                                                     \
    for (k = 0; k < _c->n * _c->n; ++k)                                        \
        _a[k] = (T)d_tests_point(g_eig_entries[_c->first + k].m,               \
                                 g_eig_entries[_c->first + k].e);              \
}                                                                              \
                                                                               \
/* the driver, both forms, on a copy of _a; whether they agree */              \
static bool                                                                    \
d_tests_eigen_both_##S(const T* _a, size_t _n, struct d_cplx_##S* _v,          \
                       struct d_cplx_##S* _x, bool* _ok, size_t _max_iter)     \
{                                                                              \
    T w1[144], w2[144], work1[192], work2[192];                                \
    struct d_cplx_##S v2[12], x2[144];                                         \
    size_t p1[12], p2[12], i;                                                  \
    bool ok2, same;                                                            \
    memcpy(w1, _a, sizeof(T) * _n * _n);                                       \
    memcpy(w2, _a, sizeof(T) * _n * _n);                                       \
    *_ok = d_decomp_##S##_eigen(w1, _n, _v, _x, work1, p1, _max_iter, (T)0);   \
    ok2 = d_decomp_##S##_eigen_c(w2, _n, v2, x2, work2, p2, _max_iter, (T)0);  \
    same = (*_ok == ok2);                                                      \
    for (i = 0; i < _n; ++i) same = same && d_tests_csame_##S(_v[i], v2[i]);   \
    for (i = 0; i < _n * _n; ++i)                                              \
        same = same && d_tests_csame_##S(_x[i], x2[i]);                        \
    return same;                                                               \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_schur_case_##S(const struct dj_tests_eig_case* _c)                      \
{                                                                              \
    const size_t n = (size_t)_c->n;                                            \
    const long double eps = (long double)EPS;                                  \
    T a[144], b[144], bc[144], xb[144], h[144], hc[144], q[144], qc[144];      \
    T t[144], tc[144], z[144], zc[144], vr[144], vrc[144], sa[144];            \
    T sv[12], dcol[12], scale[12], scalec[12], rv[12], one[12];                \
    T w3[36], w3c[36];                                                         \
    struct d_cplx_##S v[12], x[144], v3[12], x3[144], sval[12], svalc[12];     \
    size_t perm[12], permc[12], rowof[12], ilo = 0, ihi = 0, iloc, ihic;       \
    long double norm, norm_b, worst = 0.0L;                                    \
    bool used[12], conv = false, conv3 = false, ok, okc;                       \
    size_t i, j, k, m;                                                         \
                                                                               \
    D_TESTS_EXPECT(n <= 12, #S " eigen: the case's order at most 12");         \
    if (n > 12) return;                                                        \
    d_tests_case_##S(_c, a);                                                   \
    norm = d_tests_fro_ld_##S(a, n);                                           \
    if (norm == 0.0L) norm = 1.0L;                                             \
                                                                               \
    /* the driver, both forms */                                               \
    D_TESTS_EXPECT(d_tests_eigen_both_##S(a, n, v, x, &conv, 1000u),           \
                   #S " eigen_c is eigen, bit for bit");                       \
    D_TESTS_EXPECT(conv, #S " eigen: converged");                              \
                                                                               \
    /* ascending; a real eigenvalue's imaginary part +0; a pair */             \
    /* exactly conjugate, its vectors conjugate too             */             \
    ok = true;                                                                 \
    for (k = 1; k < n; ++k)                                                    \
        ok = ok && ((v[k - 1].part[0] < v[k].part[0]) ||                       \
                    ((v[k - 1].part[0] == v[k].part[0]) &&                     \
                     (v[k - 1].part[1] <= v[k].part[1])));                     \
    D_TESTS_EXPECT(ok, #S " eigen: ascending, by real part then imaginary");   \
    ok = true;                                                                 \
    for (k = 0; k < n; ++k)                                                    \
    {                                                                          \
        if (v[k].part[1] == (T)0)                                              \
        {                                                                      \
            ok = ok && d_tests_plus_zero_##S(v[k].part[1]);                    \
            for (i = 0; i < n; ++i)                                            \
                ok = ok && d_tests_plus_zero_##S(x[i * n + k].part[1]);        \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            bool partner = false;                                              \
            for (m = 0; m < n; ++m)                                            \
            {                                                                  \
                bool p = (v[m].part[0] == v[k].part[0]) &&                     \
                         (v[m].part[1] == -v[k].part[1]);                      \
                for (i = 0; p && (i < n); ++i)                                 \
                    p = (x[i * n + m].part[0] == x[i * n + k].part[0]) &&      \
                        (x[i * n + m].part[1] == -x[i * n + k].part[1]);       \
                partner = partner || p;                                        \
            }                                                                  \
            ok = ok && partner;                                                \
        }                                                                      \
    }                                                                          \
    D_TESTS_EXPECT(ok, #S " eigen: a real eigenvalue's vector real, +0; a "    \
                       "pair and its vectors conjugate");                      \
                                                                               \
    /* each eigenvalue against the nearest reference not yet taken */          \
    for (k = 0; k < n; ++k) used[k] = false;                                   \
    for (k = 0; k < n; ++k)                                                    \
    {                                                                          \
        long double best = -1.0L, cond = 1.0L;                                 \
        size_t pick = 0;                                                       \
        for (m = 0; m < n; ++m)                                                \
        {                                                                      \
            const struct dj_tests_eig_value* r = &g_eig_values[_c->value + m];  \
            long double dr, di, d;                                             \
            if (used[m]) continue;                                             \
            dr = (long double)v[k].part[0] - d_tests_text(r->re);              \
            di = (long double)v[k].part[1] - d_tests_text(r->im);              \
            d  = d_math_ld_sqrt(dr * dr + di * di);                            \
            if ((best < 0.0L) || (d < best))                                   \
            {                                                                  \
                best = d;                                                      \
                pick = m;                                                      \
                cond = d_tests_text(r->cond);                                  \
            }                                                                  \
        }                                                                      \
        used[pick] = true;                                                     \
        best = best / (eps * norm * cond);                                     \
        if (best > worst) worst = best;                                        \
    }                                                                          \
    if (worst > g_worst_value_##S) g_worst_value_##S = worst;                  \
    D_TESTS_EXPECT(worst <= (long double)VALUE_BOUND,                          \
                   #S " eigen: each eigenvalue the reference's, to its "       \
                      "condition");                                            \
                                                                               \
    /* the balancing, both forms: exact, X^-1 A X with X = P D the */          \
    /* balancing undone on the identity, one entry in each row and */          \
    /* column; rows below ihi and columns above ilo triangular     */          \
    memcpy(b, a, sizeof(T) * n * n);                                           \
    memcpy(bc, a, sizeof(T) * n * n);                                          \
    d_decomp_##S##_balance(b, n, perm, scale, &ilo, &ihi);                     \
    d_decomp_##S##_balance_c(bc, n, permc, scalec, &iloc, &ihic);              \
    ok = (ilo == iloc) && (ihi == ihic);                                       \
    for (i = 0; i < n; ++i)                                                    \
        ok = ok && (perm[i] == permc[i]) &&                                    \
             d_tests_same_##S(scale[i], scalec[i]);                            \
    for (i = 0; i < n * n; ++i) ok = ok && d_tests_same_##S(b[i], bc[i]);      \
    D_TESTS_EXPECT(ok, #S " balance_c is balance, bit for bit");               \
    for (i = 0; i < n * n; ++i) xb[i] = ((i / n) == (i % n)) ? (T)1 : (T)0;    \
    d_decomp_##S##_balance_back(xb, n, n, perm, scale, ilo, ihi);              \
    ok = (ilo <= ihi) && (ihi < n);                                            \
    for (j = 0; j < n; ++j)                                                    \
    {                                                                          \
        size_t found = 0;                                                      \
        for (i = 0; i < n; ++i)                                                \
            if (xb[i * n + j] != (T)0)                                         \
            {                                                                  \
                ++found;                                                       \
                rowof[j] = i;                                                  \
                dcol[j]  = xb[i * n + j];                                      \
                sv[i]    = xb[i * n + j];                                      \
            }                                                                  \
        ok = ok && (found == 1);                                               \
    }                                                                          \
    for (i = 0; ok && (i < n); ++i)                                            \
        for (j = 0; j < n; ++j)                                                \
            ok = ok && (b[i * n + j] ==                                        \
                        (a[rowof[i] * n + rowof[j]] * dcol[j]) / dcol[i]);     \
    for (i = 0; i < n; ++i)                                                    \
        for (j = 0; j < i; ++j)                                                \
            if ((i > ihi) || (j < ilo)) ok = ok && (b[i * n + j] == (T)0);     \
    D_TESTS_EXPECT(ok, #S " balance: D^-1 P^T A P D exactly, isolated rows "   \
                       "and columns triangular");                              \
    norm_b = d_tests_fro_ld_##S(b, n);                                         \
                                                                               \
    /* each eigenvector: unit, its largest entry real and positive, */         \
    /* its residual the balanced matrix's few epsilons              */         \
    worst = 0.0L;                                                              \
    ok = true;                                                                 \
    for (k = 0; k < n; ++k)                                                    \
    {                                                                          \
        long double vre[12], vim[12], n2 = 0.0L, big = 0.0L, res;              \
        bool top = false;                                                      \
        for (i = 0; i < n; ++i)                                                \
        {                                                                      \
            vre[i] = (long double)x[i * n + k].part[0];                        \
            vim[i] = (long double)x[i * n + k].part[1];                        \
            n2 += vre[i] * vre[i] + vim[i] * vim[i];                           \
            if (vre[i] * vre[i] + vim[i] * vim[i] > big)                       \
                big = vre[i] * vre[i] + vim[i] * vim[i];                       \
        }                                                                      \
        for (i = 0; i < n; ++i)                                                \
            top = top || ((x[i * n + k].part[1] == (T)0) &&                    \
                          (vre[i] > 0.0L) &&                                   \
                          (vre[i] * vre[i] >=                                  \
                           big * (1.0L - (long double)(8 * n) * eps)));        \
        ok = ok && top &&                                                      \
             (d_tests_abs_##S((T)(n2 - 1.0L)) <= (T)(4 * n) * EPS);            \
        res = d_tests_residual_##S(a, n, vre, vim, (long double)v[k].part[0],  \
                                   (long double)v[k].part[1], sv, norm_b);     \
        if (res > worst) worst = res;                                          \
    }                                                                          \
    if (worst > g_worst_vector_##S) g_worst_vector_##S = worst;                \
    D_TESTS_EXPECT(ok, #S " eigen: each vector unit, its largest entry real "  \
                       "and positive");                                        \
    D_TESTS_EXPECT(worst <= (long double)VECTOR_BOUND,                         \
                   #S " eigen: each vector's residual, the balanced "          \
                      "matrix's few epsilons");                                \
                                                                               \
    /* 2^SHIFT A and 2^-SHIFT A: 2^SHIFT times A's values, the same */         \
    /* vectors, bit for bit                                          */        \
    for (m = 0; m < 2; ++m)                                                    \
    {                                                                          \
        const T p2 = d_internal_math_##S##_pow2((m == 0) ? SHIFT : -SHIFT);    \
        for (i = 0; i < n * n; ++i) sa[i] = a[i] * p2;                         \
        ok = d_tests_eigen_both_##S(sa, n, v3, x3, &conv3, 1000u) &&           \
             (conv3 == conv);                                                  \
        for (k = 0; k < n; ++k)                                                \
            ok = ok && d_tests_same_##S(v3[k].part[0], v[k].part[0] * p2) &&   \
                 d_tests_same_##S(v3[k].part[1], v[k].part[1] * p2);           \
        for (i = 0; i < n * n; ++i)                                            \
            ok = ok && d_tests_csame_##S(x3[i], x[i]);                         \
        D_TESTS_EXPECT(ok, #S " eigen of 2^k A: 2^k times A's values, its "    \
                           "vectors, bit for bit");                            \
    }                                                                          \
                                                                               \
    /* mat.h's solve of [A | B], B A's first two columns or the zero  */       \
    /* ones: each column decomp.h's LU and its solve, bit for bit; a  */       \
    /* matrix LU calls singular no solution, the zero matrix           */      \
    {                                                                          \
        T lu[144], bb[24], xx[24], scr[144], col[12], xc[12], sign = (T)0;     \
        size_t pv[12];                                                         \
        bool singular, solved;                                                 \
        for (i = 0; i < n; ++i)                                                \
            for (j = 0; j < 2; ++j)                                            \
                bb[i * 2 + j] = (j < n) ? a[i * n + j] : (T)0;                 \
        memcpy(lu, a, sizeof(T) * n * n);                                      \
        singular = d_decomp_##S##_lu(lu, n, pv, &sign);                        \
        solved   = d_mat_##S##_solve(xx, scr, a, bb, n, 2);                    \
        ok = (solved == !singular);                                            \
        for (j = 0; ok && (j < 2); ++j)                                        \
        {                                                                      \
            for (i = 0; i < n; ++i) col[i] = bb[i * 2 + j];                    \
            if (!singular) d_decomp_##S##_lu_solve(lu, pv, n, col, xc);        \
            for (i = 0; i < n; ++i)                                            \
                ok = ok && d_tests_same_##S(xx[i * 2 + j],                     \
                                            singular ? (T)0 : xc[i]);          \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " mat solve: LU's, bit for bit, column by "      \
                           "column");                                          \
    }                                                                          \
                                                                               \
    /* the old eigenvalues: the driver's real parts, found where all */        \
    /* are real                                                      */        \
    {                                                                          \
        T w[144];                                                              \
        bool real = true, okv;                                                 \
        memcpy(w, a, sizeof(T) * n * n);                                       \
        okv = d_decomp_##S##_eigenvalues(w, n, rv, 1000u, (T)0);               \
        for (k = 0; k < n; ++k) real = real && (v[k].part[1] == (T)0);         \
        ok = (okv == (conv && real));                                          \
        for (k = 0; k < n; ++k)                                                \
            ok = ok && d_tests_same_##S(rv[k], v[k].part[0]);                  \
        D_TESTS_EXPECT(ok, #S " eigenvalues: the driver's real parts");        \
    }                                                                          \
                                                                               \
    /* the Hessenberg form of the balanced matrix, both forms */               \
    memcpy(h, b, sizeof(T) * n * n);                                           \
    memcpy(hc, b, sizeof(T) * n * n);                                          \
    for (i = 0; i < n * n; ++i)                                                \
        q[i] = qc[i] = ((i / n) == (i % n)) ? (T)1 : (T)0;                     \
    d_decomp_##S##_hessenberg_q(h, n, ilo, ihi, q);                            \
    d_decomp_##S##_hessenberg_q_c(hc, n, ilo, ihi, qc);                        \
    ok = true;                                                                 \
    for (i = 0; i < n * n; ++i)                                                \
        ok = ok && d_tests_same_##S(h[i], hc[i]) &&                            \
             d_tests_same_##S(q[i], qc[i]);                                    \
    D_TESTS_EXPECT(ok, #S " hessenberg_q_c is hessenberg_q, bit for bit");     \
    ok = true;                                                                 \
    for (i = 0; i < n; ++i)                                                    \
        for (j = 0; j + 1 < i; ++j) ok = ok && (h[i * n + j] == (T)0);         \
    worst = d_tests_orthogonality_##S(q, n) / (eps * (long double)n);          \
    if (d_tests_similar_##S(q, h, b, n) / (eps * (long double)n * norm_b) >    \
        worst)                                                                 \
        worst = d_tests_similar_##S(q, h, b, n) /                              \
                (eps * (long double)n * norm_b);                               \
    if (worst > g_worst_schur_##S) g_worst_schur_##S = worst;                  \
    D_TESTS_EXPECT(ok && (worst <= (long double)SCHUR_BOUND),                  \
                   #S " hessenberg_q: zero below, Q orthogonal, Q H Q^T B");   \
                                                                               \
    /* the Schur form, both forms */                                           \
    memcpy(t, h, sizeof(T) * n * n);                                           \
    memcpy(tc, h, sizeof(T) * n * n);                                          \
    memcpy(z, q, sizeof(T) * n * n);                                           \
    memcpy(zc, q, sizeof(T) * n * n);                                          \
    ok  = d_decomp_##S##_schur(t, n, ilo, ihi, z, sval, 1000u, (T)0);          \
    okc = d_decomp_##S##_schur_c(tc, n, ilo, ihi, zc, svalc, 1000u, (T)0);     \
    okc = (ok == okc);                                                         \
    for (i = 0; i < n * n; ++i)                                                \
        okc = okc && d_tests_same_##S(t[i], tc[i]) &&                          \
              d_tests_same_##S(z[i], zc[i]);                                   \
    for (i = 0; i < n; ++i) okc = okc && d_tests_csame_##S(sval[i], svalc[i]); \
    D_TESTS_EXPECT(okc, #S " schur_c is schur, bit for bit");                  \
    /* quasi-triangular, each block of two standard, its values the */         \
    /* blocks' -- a pair positive first -- and the diagonal's        */        \
    for (i = 0; i < n; ++i)                                                    \
        for (j = 0; j + 1 < i; ++j) ok = ok && (t[i * n + j] == (T)0);         \
    for (i = 0; i < n; )                                                       \
    {                                                                          \
        if ((i + 1 < n) && (t[(i + 1) * n + i] != (T)0))                       \
        {                                                                      \
            const T up = t[i * n + i + 1];                                     \
            const T lo = t[(i + 1) * n + i];                                   \
            ok = ok && (t[i * n + i] == t[(i + 1) * n + i + 1]) &&             \
                 (((up < (T)0) && (lo > (T)0)) ||                              \
                  ((up > (T)0) && (lo < (T)0))) &&                             \
                 ((i + 2 >= n) || (t[(i + 2) * n + i + 1] == (T)0)) &&         \
                 (sval[i].part[0] == t[i * n + i]) &&                          \
                 (sval[i + 1].part[0] == t[i * n + i]) &&                      \
                 (sval[i].part[1] > (T)0) &&                                   \
                 (sval[i + 1].part[1] == -sval[i].part[1]);                    \
            i += 2;                                                            \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            ok = ok && (sval[i].part[0] == t[i * n + i]) &&                    \
                 d_tests_plus_zero_##S(sval[i].part[1]);                       \
            i += 1;                                                            \
        }                                                                      \
    }                                                                          \
    worst = d_tests_orthogonality_##S(z, n) / (eps * (long double)n);          \
    if (d_tests_similar_##S(z, t, b, n) / (eps * (long double)n * norm_b) >    \
        worst)                                                                 \
        worst = d_tests_similar_##S(z, t, b, n) /                              \
                (eps * (long double)n * norm_b);                               \
    if (worst > g_worst_schur_##S) g_worst_schur_##S = worst;                  \
    D_TESTS_EXPECT(ok && (worst <= (long double)SCHUR_BOUND),                  \
                   #S " schur: quasi-triangular, standard blocks, Z "          \
                      "orthogonal, Z T Z^T B");                                \
                                                                               \
    /* the Schur vectors, both forms: eigenvectors of B, a pair's */           \
    /* as its real and imaginary parts in two columns              */          \
    memcpy(vr, z, sizeof(T) * n * n);                                          \
    memcpy(vrc, z, sizeof(T) * n * n);                                         \
    d_decomp_##S##_schur_vectors(t, n, vr, w3);                                \
    d_decomp_##S##_schur_vectors_c(t, n, vrc, w3c);                            \
    ok = true;                                                                 \
    for (i = 0; i < n * n; ++i) ok = ok && d_tests_same_##S(vr[i], vrc[i]);    \
    D_TESTS_EXPECT(ok, #S " schur_vectors_c is schur_vectors, bit for bit");   \
    worst = 0.0L;                                                              \
    for (i = 0; i < n; ++i) one[i] = (T)1;                                     \
    for (k = 0; k < n; )                                                       \
    {                                                                          \
        long double vre[12], vim[12], res;                                     \
        const bool pair = (sval[k].part[1] != (T)0);                           \
        for (i = 0; i < n; ++i)                                                \
        {                                                                      \
            vre[i] = (long double)vr[i * n + k];                               \
            vim[i] = pair ? (long double)vr[i * n + k + 1] : 0.0L;             \
        }                                                                      \
        res = d_tests_residual_##S(b, n, vre, vim,                             \
                                   (long double)sval[k].part[0],               \
                                   (long double)sval[k].part[1], one,          \
                                   norm_b);                                    \
        if (res > worst) worst = res;                                          \
        k += pair ? 2 : 1;                                                     \
    }                                                                          \
    if (worst > g_worst_vector_##S) g_worst_vector_##S = worst;                \
    D_TESTS_EXPECT(worst <= (long double)VECTOR_BOUND,                         \
                   #S " schur_vectors: eigenvectors of Z T Z^T");              \
}                                                                              \
                                                                               \
/* the Jacobi tangent past 2^(EMAX / 2), where theta^2 would overflow: */      \
/* the rotation 1 / (2 theta), sym_eigen's and the SVD's, at a          */     \
/* threshold of 0; before, none, and a constant expression's error      */     \
static void                                                                    \
d_tests_jacobi_far_##S(int _emax)                                              \
{                                                                              \
    const T e = d_internal_math_##S##_pow2(-(_emax / 2) - 8);                  \
    volatile T want = (T)0;                                                    \
    T w[4], val[2], vec[4], u[4], sv[2], v[4];                                 \
    w[0] = (T)1;                                                               \
    w[1] = e;                                                                  \
    w[2] = e;                                                                  \
    w[3] = (T)2;                                                               \
    vec[0] = vec[3] = v[0] = v[3] = (T)1;                                      \
    vec[1] = vec[2] = v[1] = v[2] = (T)0;                                      \
    d_decomp_##S##_sym_eigen(w, val, vec, 2, (T)0);                            \
    D_TESTS_EXPECT((val[0] == (T)1) && (val[1] == (T)2) &&                     \
                   (d_tests_abs_##S(vec[1]) == e) &&                           \
                   (d_tests_abs_##S(vec[2]) == e) &&                           \
                   (vec[0] == (T)1) && (vec[3] == (T)1),                       \
                   #S " sym_eigen: the tangent past 2^(EMAX/2), a / diff");    \
    /* columns (1, e) and (0, 2): gamma 2 e, diff 3; V's columns */            \
    /* swapped, the larger singular value first                   */           \
    w[0] = (T)1;                                                               \
    w[1] = (T)0;                                                               \
    w[2] = e;                                                                  \
    w[3] = (T)2;                                                               \
    d_decomp_##S##_svd(w, u, sv, v, 2, 2, (T)0);                               \
    /* the tangent stored, so that the x87 compares it as stored */            \
    want = ((T)2 * e) / (T)3;                                                  \
    D_TESTS_EXPECT((d_tests_abs_##S(sv[0] - (T)2) <= (T)4 * EPS) &&            \
                   (d_tests_abs_##S(sv[1] - (T)1) <= (T)4 * EPS) &&            \
                   (d_tests_abs_##S(v[0]) == want) &&                          \
                   (d_tests_abs_##S(v[3]) == want) &&                          \
                   (v[1] == (T)1) && (v[2] == (T)1),                           \
                   #S " svd: the tangent past 2^(EMAX/2), gamma / diff");      \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_schur_##S(void)                                                        \
{                                                                              \
    const T eps = EPS;                                                         \
    size_t c, i;                                                               \
                                                                               \
    for (c = 0; c < sizeof(g_eig_cases) / sizeof(g_eig_cases[0]); ++c)         \
        d_tests_schur_case_##S(&g_eig_cases[c]);                               \
    d_tests_jacobi_far_##S(EMAX);                                              \
                                                                               \
    /* matrices with known answers */                                          \
    {                                                                          \
        const T zero3[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };                      \
        const T id3[9]   = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };                      \
        const T cyc3[9]  = { 0, 1, 0, 0, 0, 1, 1, 0, 0 };                      \
        const T jord2[4] = { 1, 1, -1, 3 };                                    \
        const T big2[4]  = { 0, -BIG, BIG, 0 };                                \
        const T tiny2[4] = { 0, -SMALL, SMALL, 0 };                            \
        T bad[9];                                                              \
        struct d_cplx_##S v[12], x[144];                                       \
        bool conv = false, ok, nan = true;                                     \
                                                                               \
        ok = d_tests_eigen_both_##S(zero3, 0, v, x, &conv, 1000u);             \
        D_TESTS_EXPECT(ok && conv, #S " eigen: order 0, nothing to find");     \
        ok = d_tests_eigen_both_##S(zero3, 3, v, x, &conv, 1000u) && conv;     \
        for (i = 0; i < 3; ++i)                                                \
            ok = ok && (v[i].part[0] == (T)0) && (v[i].part[1] == (T)0);       \
        for (i = 0; i < 9; ++i)                                                \
            ok = ok && (x[i].part[0] == id3[i]) && (x[i].part[1] == (T)0);     \
        D_TESTS_EXPECT(ok, #S " eigen: the zero matrix, the identity's "       \
                           "columns");                                         \
        ok = d_tests_eigen_both_##S(id3, 3, v, x, &conv, 1000u) && conv;       \
        for (i = 0; i < 3; ++i) ok = ok && (v[i].part[0] == (T)1);             \
        for (i = 0; i < 9; ++i) ok = ok && (x[i].part[0] == id3[i]);           \
        D_TESTS_EXPECT(ok, #S " eigen: the identity, exactly");                \
        ok = d_tests_eigen_both_##S(jord2, 2, v, x, &conv, 1000u) && conv;     \
        for (i = 0; i < 2; ++i)                                                \
            ok = ok && (d_tests_abs_##S(v[i].part[0] - (T)2) <=                \
                        (T)8 * d_math_##S##_sqrt(eps) * (T)4) &&               \
                 (d_tests_abs_##S(v[i].part[1]) <=                             \
                  (T)8 * d_math_##S##_sqrt(eps) * (T)4);                       \
        D_TESTS_EXPECT(ok, #S " eigen: a Jordan block, 2 to sqrt(eps)");       \
        ok = d_tests_eigen_both_##S(big2, 2, v, x, &conv, 1000u) && conv &&    \
             (v[0].part[0] == (T)0) && (v[0].part[1] == -BIG) &&               \
             (v[1].part[0] == (T)0) && (v[1].part[1] == BIG);                  \
        D_TESTS_EXPECT(ok, #S " eigen: +-i times the largest power of two, "   \
                           "exactly");                                         \
        ok = d_tests_eigen_both_##S(tiny2, 2, v, x, &conv, 1000u) && conv &&   \
             (v[0].part[0] == (T)0) && (v[0].part[1] == -SMALL) &&             \
             (v[1].part[0] == (T)0) && (v[1].part[1] == SMALL);                \
        D_TESTS_EXPECT(ok, #S " eigen: +-i times the smallest normal, "        \
                           "exactly");                                         \
        ok = d_tests_eigen_both_##S(cyc3, 3, v, x, &conv, 0u) && !conv;        \
        for (i = 0; i < 9; ++i)                                                \
            nan = nan && (x[i].part[0] != x[i].part[0]) &&                     \
                  (x[i].part[1] != x[i].part[1]);                              \
        D_TESTS_EXPECT(ok && nan, #S " eigen: no steps allowed, not "          \
                                     "converged, NaN vectors");                \
        memcpy(bad, id3, sizeof(bad));                                         \
        bad[5] = d_internal_math_##S##_nan();                                  \
        ok = d_tests_eigen_both_##S(bad, 3, v, x, &conv, 1000u) && !conv;      \
        for (i = 0; i < 3; ++i)                                                \
            ok = ok && (v[i].part[0] != v[i].part[0]) &&                       \
                 (v[i].part[1] != v[i].part[1]);                               \
        for (i = 0; i < 9; ++i) ok = ok && (x[i].part[0] != x[i].part[0]);     \
        D_TESTS_EXPECT(ok, #S " eigen: a NaN, every result NaN, flagged");     \
        bad[5] = -(T)HUGE_VAL;                                                 \
        ok = d_tests_eigen_both_##S(bad, 3, v, x, &conv, 1000u) && !conv &&    \
             (v[1].part[0] != v[1].part[0]);                                   \
        D_TESTS_EXPECT(ok, #S " eigen: an infinity, every result NaN, "        \
                           "flagged");                                         \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_SCHUR_FAMILY(float, f, FLT_EPSILON, FLT_MAX_EXP, 30,
                     d_internal_math_f_pow2(FLT_MAX_EXP - 1), FLT_MIN,
                     32, 32, 8)
D_TESTS_SCHUR_FAMILY(double, d, DBL_EPSILON, DBL_MAX_EXP, 300,
                     d_internal_math_d_pow2(DBL_MAX_EXP - 1), DBL_MIN,
                     32, 32, 8)
D_TESTS_SCHUR_FAMILY(long double, ld, LDBL_EPSILON, LDBL_MAX_EXP, 3000,
                     d_internal_math_ld_pow2(LDBL_MAX_EXP - 1), LDBL_MIN,
                     32, 32, 8)

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

/* from C++14, d_decomp_<S>_eigen_c of a 3-by-3 matrix with a complex    */
/* pair as a constant expression -- its values' parts and its first      */
/* vector's, one call apiece -- against the run time's, bit for bit; and */
/* sym_eigen_c at a threshold of 0 where theta^2 would overflow          */
#define D_TESTS_SCHUR_CONSTANT(T, S, EMAX)                                     \
static constexpr T                                                             \
d_tests_constant_part_##S(size_t _k)                                           \
{                                                                              \
    T a[9] = { 1, -2, 0, 3, 1, 1, 0, 1, 2 };                                   \
    T work[21] = {};                                                           \
    struct d_cplx_##S v[3] = {};                                               \
    struct d_cplx_##S x[9] = {};                                               \
    size_t perm[3] = { 0, 0, 0 };                                              \
    (void)d_decomp_##S##_eigen_c(a, 3, v, x, work, perm, 1000u, (T)0);         \
    return (_k < 6) ? v[_k / 2].part[_k % 2]                                   \
                    : x[((_k - 6) / 2) * 3].part[(_k - 6) % 2];                \
}                                                                              \
                                                                               \
static constexpr T g_constant_##S[12] =                                        \
{                                                                              \
    d_tests_constant_part_##S(0), d_tests_constant_part_##S(1),                \
    d_tests_constant_part_##S(2), d_tests_constant_part_##S(3),                \
    d_tests_constant_part_##S(4), d_tests_constant_part_##S(5),                \
    d_tests_constant_part_##S(6), d_tests_constant_part_##S(7),                \
    d_tests_constant_part_##S(8), d_tests_constant_part_##S(9),                \
    d_tests_constant_part_##S(10), d_tests_constant_part_##S(11)               \
};                                                                             \
                                                                               \
static constexpr T                                                             \
d_tests_constant_jacobi_##S(size_t _k)                                         \
{                                                                              \
    T w[4] = { 1, 0, 0, 2 };                                                   \
    T val[2] = {};                                                             \
    T vec[4] = { 1, 0, 0, 1 };                                                 \
    w[1] = w[2] = d_internal_math_##S##_pow2(-(EMAX / 2) - 8);                 \
    d_decomp_##S##_sym_eigen_c(w, val, vec, 2, (T)0);                          \
    return (_k < 2) ? val[_k] : vec[_k - 1];                                   \
}                                                                              \
                                                                               \
static constexpr T g_constant_jacobi_##S[3] =                                  \
{                                                                              \
    d_tests_constant_jacobi_##S(0), d_tests_constant_jacobi_##S(1),            \
    d_tests_constant_jacobi_##S(2)                                             \
};                                                                             \
                                                                               \
static void                                                                    \
d_tests_schur_constant_##S(void)                                               \
{                                                                              \
    volatile T src[9] = { 1, -2, 0, 3, 1, 1, 0, 1, 2 };                        \
    T a[9], work[21];                                                          \
    struct d_cplx_##S v[3], x[9];                                              \
    size_t perm[3], k;                                                         \
    bool ok = true;                                                            \
    for (k = 0; k < 9; ++k) a[k] = src[k];                                     \
    (void)d_decomp_##S##_eigen(a, 3, v, x, work, perm, 1000u, (T)0);           \
    for (k = 0; k < 6; ++k)                                                    \
        ok = ok && d_tests_same_##S(g_constant_##S[k], v[k / 2].part[k % 2]);  \
    for (k = 6; k < 12; ++k)                                                   \
        ok = ok && d_tests_same_##S(g_constant_##S[k],                         \
                                    x[((k - 6) / 2) * 3].part[(k - 6) % 2]);   \
    D_TESTS_EXPECT(ok && (v[1].part[1] != (T)0),                               \
                   #S " eigen_c: a constant expression, the run time's bits"); \
    D_TESTS_EXPECT((g_constant_jacobi_##S[0] == (T)1) &&                       \
                   (g_constant_jacobi_##S[1] == (T)2) &&                       \
                   (d_tests_abs_##S(g_constant_jacobi_##S[2]) ==               \
                    d_internal_math_##S##_pow2(-(EMAX / 2) - 8)),              \
                   #S " sym_eigen_c: a constant expression at a threshold "    \
                      "of 0, the tangent past 2^(EMAX/2)");                    \
}

D_TESTS_SCHUR_CONSTANT(float,       f,  FLT_MAX_EXP)
D_TESTS_SCHUR_CONSTANT(double,      d,  DBL_MAX_EXP)
D_TESTS_SCHUR_CONSTANT(long double, ld, LDBL_MAX_EXP)

#endif

int main(void)
{
    d_tests_decomp_f();
    d_tests_decomp_d();
    d_tests_decomp_ld();
    /* last, so every earlier draw is as it was */
    d_tests_eigen_f();
    d_tests_eigen_d();
    d_tests_eigen_ld();
    /* the references, which draw nothing */
    d_tests_schur_f();
    d_tests_schur_d();
    d_tests_schur_ld();
    printf("  eigen, the worst in epsilons: values %.3Lg %.3Lg %.3Lg (of "
           "|A|_F cond); vectors %.3Lg %.3Lg %.3Lg (of the balanced "
           "|B|_F); Schur %.3Lg %.3Lg %.3Lg (of n |B|_F)\n",
           g_worst_value_f, g_worst_value_d, g_worst_value_ld,
           g_worst_vector_f, g_worst_vector_d, g_worst_vector_ld,
           g_worst_schur_f, g_worst_schur_d, g_worst_schur_ld);
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_schur_constant_f();
    d_tests_schur_constant_d();
    d_tests_schur_constant_ld();
#endif

    printf("t_decomp: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
