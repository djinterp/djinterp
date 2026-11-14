/*******************************************************************************
* djinterp [c/math]                                                   t_decomp.c
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
* sym_eigen's and the SVD's. The largest measures are printed. Then the
* guards, each path the parts take at the range's ends and at nearly
* singular systems reached on purpose: dlaln2's and dlanv2's every branch,
* the scaled norms, the reflector below the safe minimum, the exceptional
* shifts, the vectors' scalings and a vector underflowing, balancing across
* the range -- D^-1 P^T A P D exactly where no entry passes below the least
* normal on the way -- and the _c forms of the tangent. From C++14,
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
*                                                            revised: 2026.11.07
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
#include "./math_tests_eig_refs.h"                               // the references


static long          g_checks   = 0;
static long          g_failures = 0;
static unsigned long g_state    = 2236067u;

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

// d_tests_point
//   m 2^e, exactly, as the references give an entry.
static long double
d_tests_point(
    long _m,
    int  _e
)
{
    return (long double)_m * d_internal_math_ld_pow2(_e);
}

// d_tests_text
//   the decimal _text, a reference's, rounded to long double by decimal.h.
static long double
d_tests_text(
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
d_tests_csame_##S(struct dj_cplx_##S _x, struct dj_cplx_##S _y)                \
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
d_tests_case_##S(const struct dj_tests_eig_case* _c, T* _a)                    \
{                                                                              \
    int k;                                                                     \
    for (k = 0; k < _c->n * _c->n; ++k)                                        \
        _a[k] = (T)d_tests_point(g_eig_entries[_c->first + k].m,               \
                                 g_eig_entries[_c->first + k].e);              \
}                                                                              \
                                                                               \
/* the driver, both forms, on a copy of _a; whether they agree */              \
static bool                                                                    \
d_tests_eigen_both_##S(const T* _a, size_t _n, struct dj_cplx_##S* _v,         \
                       struct dj_cplx_##S* _x, bool* _ok, size_t _max_iter)    \
{                                                                              \
    T w1[144], w2[144], work1[192], work2[192];                                \
    struct dj_cplx_##S v2[12], x2[144];                                        \
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
d_tests_schur_case_##S(const struct dj_tests_eig_case* _c)                     \
{                                                                              \
    const size_t n = (size_t)_c->n;                                            \
    const long double eps = (long double)EPS;                                  \
    T a[144], b[144], bc[144], xb[144], h[144], hc[144], q[144], qc[144];      \
    T t[144], tc[144], z[144], zc[144], vr[144], vrc[144], sa[144];            \
    T sv[12], dcol[12], scale[12], scalec[12], rv[12], one[12];                \
    T w3[36], w3c[36];                                                         \
    struct dj_cplx_##S v[12], x[144], v3[12], x3[144], sval[12], svalc[12];    \
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
            const struct dj_tests_eig_value* r = &g_eig_values[_c->value + m]; \
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
        struct dj_cplx_##S v[12], x[144];                                      \
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

// D_TESTS_GUARD_FAMILY
//   stamps, for one family, d_tests_guard_<S>: the paths dgeev's parts take
// at the range's ends and at nearly singular systems, each reached on
// purpose and checked -- dlaln2's small systems, dlanv2's blocks of two,
// the scaled norms, the reflector below the safe minimum, a NaN's hypot and
// the parts of order 0, a subnormal matrix, Hessenberg matrices found to
// need the exceptional shifts, the Schur vectors' scalings and a vector
// underflowing, balancing across the range, the Jacobi tangent's _c forms
// at run time, and paths of earlier chunks'. TMIN is the family's least
// normal number and EMAX its largest exponent.
#define D_TESTS_GUARD_FAMILY(T, S, EPS, TMIN, EMAX)                            \
/* a case of dlaln2's: (T - w I) X = scale B, 1 by 1 or 2 by 2 */              \
struct dj_tests_solve_##S                                                      \
{                                                                              \
    size_t na, nw;                                                             \
    T      smin, t[4], br[2], bi[2], wr, wi;                                   \
    bool   scaled;                                                             \
};                                                                             \
                                                                               \
/* whether _x is finite */                                                     \
static bool                                                                    \
d_tests_finite_##S(T _x)                                                       \
{                                                                              \
    return (_x - _x) == (T)0;                                                  \
}                                                                              \
                                                                               \
/* the n-by-n _h's Schur form, both forms, Z from the identity: the two */     \
/* alike, converged, quasi-triangular with standard blocks, Z          */      \
/* orthogonal and Z T Z^T _h to 8 n epsilons                            */     \
static bool                                                                    \
d_tests_guard_schur_##S(const T* _h, size_t _n)                                \
{                                                                              \
    T t[144], tc[144], z[144], zc[144];                                        \
    struct dj_cplx_##S v[12], vc[12];                                          \
    long double norm = d_tests_fro_ld_##S(_h, _n), worst;                      \
    bool ok, okc, form;                                                        \
    size_t i, j;                                                               \
    memcpy(t, _h, sizeof(T) * _n * _n);                                        \
    memcpy(tc, _h, sizeof(T) * _n * _n);                                       \
    for (i = 0; i < _n * _n; ++i)                                              \
        z[i] = zc[i] = ((i / _n) == (i % _n)) ? (T)1 : (T)0;                   \
    ok  = d_decomp_##S##_schur(t, _n, 0, _n - 1, z, v, 1000u, (T)0);           \
    okc = d_decomp_##S##_schur_c(tc, _n, 0, _n - 1, zc, vc, 1000u, (T)0);      \
    form = (ok == okc);                                                        \
    for (i = 0; i < _n * _n; ++i)                                              \
        form = form && d_tests_same_##S(t[i], tc[i]) &&                        \
               d_tests_same_##S(z[i], zc[i]);                                  \
    for (i = 0; i < _n; ++i) form = form && d_tests_csame_##S(v[i], vc[i]);    \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j + 1 < i; ++j) ok = ok && (t[i * _n + j] == (T)0);        \
    for (i = 0; i + 1 < _n; ++i)                                               \
        if (t[(i + 1) * _n + i] != (T)0)                                       \
        {                                                                      \
            const T up = t[i * _n + i + 1], lo = t[(i + 1) * _n + i];          \
            ok = ok && (t[i * _n + i] == t[(i + 1) * _n + i + 1]) &&           \
                 (((up < (T)0) && (lo > (T)0)) ||                              \
                  ((up > (T)0) && (lo < (T)0))) &&                             \
                 ((i + 2 >= _n) || (t[(i + 2) * _n + i + 1] == (T)0));         \
        }                                                                      \
    if (norm == 0.0L) norm = 1.0L;                                             \
    worst = d_tests_orthogonality_##S(z, _n) /                                 \
            ((long double)EPS * (long double)_n);                              \
    if (d_tests_similar_##S(z, t, _h, _n) /                                    \
        ((long double)EPS * (long double)_n * norm) > worst)                   \
        worst = d_tests_similar_##S(z, t, _h, _n) /                            \
                ((long double)EPS * (long double)_n * norm);                   \
    return form && ok && (worst <= 8.0L);                                      \
}                                                                              \
                                                                               \
/* the quasi-triangular n-by-n _t's eigenvectors, both forms, Z the */         \
/* identity: the two alike, each column's largest magnitude 1, and  */         \
/* each vector's residual within 32 epsilons of |T|_F               */         \
static bool                                                                    \
d_tests_guard_vectors_##S(const T* _t, size_t _n)                              \
{                                                                              \
    T vr[144], vrc[144], work[36], workc[36], one[12];                         \
    long double norm = d_tests_fro_ld_##S(_t, _n), worst = 0.0L;               \
    bool ok = true;                                                            \
    size_t i, k;                                                               \
    for (i = 0; i < _n * _n; ++i)                                              \
        vr[i] = vrc[i] = ((i / _n) == (i % _n)) ? (T)1 : (T)0;                 \
    for (i = 0; i < _n; ++i) one[i] = (T)1;                                    \
    d_decomp_##S##_schur_vectors(_t, _n, vr, work);                            \
    d_decomp_##S##_schur_vectors_c(_t, _n, vrc, workc);                        \
    for (i = 0; i < _n * _n; ++i)                                              \
        ok = ok && d_tests_same_##S(vr[i], vrc[i]) &&                          \
             (d_tests_abs_##S(vr[i]) <= (T)1);                                 \
    for (k = 0; k < _n; )                                                      \
    {                                                                          \
        long double vre[12], vim[12], lr, li = 0.0L, res;                      \
        const bool pair = (k + 1 < _n) && (_t[(k + 1) * _n + k] != (T)0);      \
        lr = (long double)_t[k * _n + k];                                      \
        if (pair)                                                              \
            li = d_math_ld_sqrt((long double)                                  \
                                d_tests_abs_##S(_t[k * _n + k + 1])) *         \
                 d_math_ld_sqrt((long double)                                  \
                                d_tests_abs_##S(_t[(k + 1) * _n + k]));        \
        for (i = 0; i < _n; ++i)                                               \
        {                                                                      \
            vre[i] = (long double)vr[i * _n + k];                              \
            vim[i] = pair ? (long double)vr[i * _n + k + 1] : 0.0L;            \
        }                                                                      \
        res = d_tests_residual_##S(_t, _n, vre, vim, lr, li, one, norm);       \
        if (res > worst) worst = res;                                          \
        k += pair ? 2 : 1;                                                     \
    }                                                                          \
    return ok && (worst <= 32.0L);                                             \
}                                                                              \
                                                                               \
/* the 2-by-2 block, both forms of the standard form: alike; cs^2 + */         \
/* sn^2 one; R^T A R the block to 8 epsilons of |A|_F; upper        */         \
/* triangular, or equal diagonals and off-diagonals of opposite     */         \
/* signs; its values the block's. Measured in units of the block's  */         \
/* largest magnitude, so that nothing leaves long double's range    */         \
static bool                                                                    \
d_tests_guard_pair_##S(T _a, T _b, T _c, T _d)                                 \
{                                                                              \
    T a = _a, b = _b, c = _c, d = _d, cs, sn;                                  \
    T a2 = _a, b2 = _b, c2 = _c, d2 = _d, cs2, sn2;                            \
    struct dj_cplx_##S v[2], v2[2];                                            \
    long double in[4], out[4], m[4], r[4], err = 0.0L, norm = 0.0L;            \
    long double unit = 0.0L, lc, ls;                                           \
    volatile T im;                                                             \
    bool ok;                                                                   \
    size_t i;                                                                  \
    d_internal_decomp_##S##_standard_pair(&a, &b, &c, &d, v, &cs, &sn);        \
    d_internal_decomp_##S##_standard_pair_c(&a2, &b2, &c2, &d2, v2, &cs2,      \
                                             &sn2);                            \
    ok = d_tests_same_##S(a, a2) && d_tests_same_##S(b, b2) &&                 \
         d_tests_same_##S(c, c2) && d_tests_same_##S(d, d2) &&                 \
         d_tests_same_##S(cs, cs2) && d_tests_same_##S(sn, sn2) &&             \
         d_tests_csame_##S(v[0], v2[0]) && d_tests_csame_##S(v[1], v2[1]);     \
    lc = (long double)cs;                                                      \
    ls = (long double)sn;                                                      \
    r[0] = lc * lc + ls * ls - 1.0L;                                           \
    ok = ok && (((r[0] < 0.0L) ? -r[0] : r[0]) <= 4.0L * (long double)EPS);    \
    in[0] = (long double)_a; in[1] = (long double)_b;                          \
    in[2] = (long double)_c; in[3] = (long double)_d;                          \
    out[0] = (long double)a; out[1] = (long double)b;                          \
    out[2] = (long double)c; out[3] = (long double)d;                          \
    for (i = 0; i < 4; ++i)                                                    \
        if (((in[i] < 0.0L) ? -in[i] : in[i]) > unit)                          \
            unit = (in[i] < 0.0L) ? -in[i] : in[i];                            \
    if (unit == 0.0L) unit = 1.0L;                                             \
    for (i = 0; i < 4; ++i)                                                    \
    {                                                                          \
        in[i]  = in[i] / unit;                                                 \
        out[i] = out[i] / unit;                                                \
        norm  += in[i] * in[i];                                                \
    }                                                                          \
    /* A R, then R^T (A R), R = [cs -sn; sn cs] */                             \
    m[0] = in[0] * lc + in[1] * ls;                                            \
    m[1] = in[1] * lc - in[0] * ls;                                            \
    m[2] = in[2] * lc + in[3] * ls;                                            \
    m[3] = in[3] * lc - in[2] * ls;                                            \
    r[0] = lc * m[0] + ls * m[2] - out[0];                                     \
    r[1] = lc * m[1] + ls * m[3] - out[1];                                     \
    r[2] = lc * m[2] - ls * m[0] - out[2];                                     \
    r[3] = lc * m[3] - ls * m[1] - out[3];                                     \
    for (i = 0; i < 4; ++i) err += r[i] * r[i];                                \
    ok = ok && (d_math_ld_sqrt(err) <=                                         \
                8.0L * (long double)EPS * d_math_ld_sqrt(norm));               \
    /* the pair's imaginary part stored, so that the x87 compares it */        \
    /* as stored                                                      */       \
    im = d_math_##S##_sqrt(d_tests_abs_##S(b)) *                               \
         d_math_##S##_sqrt(d_tests_abs_##S(c));                                \
    if (c == (T)0)                                                             \
        ok = ok && (v[0].part[0] == a) && (v[1].part[0] == d) &&               \
             d_tests_plus_zero_##S(v[0].part[1]) &&                            \
             d_tests_plus_zero_##S(v[1].part[1]);                              \
    else                                                                       \
        ok = ok && (a == d) &&                                                 \
             (((b < (T)0) && (c > (T)0)) || ((b > (T)0) && (c < (T)0))) &&     \
             (v[0].part[0] == a) && (v[1].part[0] == a) &&                     \
             (v[0].part[1] == im) && (v[1].part[1] == -v[0].part[1]);          \
    return ok;                                                                 \
}                                                                              \
                                                                               \
/* a case of dlaln2's: scale in (0, 1], below 1 where expected; X */           \
/* finite; xnorm its largest magnitude, |re| + |im| where complex, */          \
/* to 2 epsilons; and each row's residual within 4 smin |X| and 8  */          \
/* epsilons of |C| |X| + scale |B|                                 */          \
static bool                                                                    \
d_tests_guard_solve_##S(const struct dj_tests_solve_##S* _c)                   \
{                                                                              \
    T xr[2] = { 0, 0 }, xi[2] = { 0, 0 }, scale = (T)0, xnorm = (T)0;          \
    const T smini = (_c->smin > (T)2 * TMIN) ? _c->smin : (T)2 * TMIN;         \
    const bool cx = (_c->nw == 2);                                             \
    long double cr[4], ci[4], big = 0.0L, gap;                                 \
    bool ok;                                                                   \
    size_t i, j;                                                               \
    d_internal_decomp_##S##_small_solve(_c->na, _c->nw, _c->smin, _c->t, 2,    \
                                         _c->br, _c->bi, _c->wr, _c->wi, xr,   \
                                         xi, &scale, &xnorm);                  \
    ok = (scale > (T)0) && (scale <= (T)1) &&                                  \
         (_c->scaled == (scale < (T)1));                                       \
    for (i = 0; i < _c->na; ++i)                                               \
    {                                                                          \
        const long double m = (long double)d_tests_abs_##S(xr[i]) +            \
                              (cx ? (long double)d_tests_abs_##S(xi[i])        \
                                  : 0.0L);                                     \
        ok = ok && d_tests_finite_##S(xr[i]) && d_tests_finite_##S(xi[i]);     \
        if (m > big) big = m;                                                  \
    }                                                                          \
    gap = (long double)xnorm - big;                                            \
    ok = ok && (((gap < 0.0L) ? -gap : gap) <=                                 \
                2.0L * (long double)EPS * big);                                \
    for (i = 0; i < _c->na; ++i)                                               \
        for (j = 0; j < _c->na; ++j)                                           \
        {                                                                      \
            cr[i * 2 + j] = (long double)_c->t[i * 2 + j] -                    \
                            ((i == j) ? (long double)_c->wr : 0.0L);           \
            ci[i * 2 + j] = ((i == j) && cx) ? -(long double)_c->wi : 0.0L;    \
        }                                                                      \
    for (i = 0; i < _c->na; ++i)                                               \
    {                                                                          \
        long double rr = -(long double)scale * (long double)_c->br[i];         \
        long double ri = cx ? -(long double)scale * (long double)_c->bi[i]     \
                            : 0.0L;                                            \
        long double cabs = 0.0L, babs, bound;                                  \
        for (j = 0; j < _c->na; ++j)                                           \
        {                                                                      \
            const long double x1 = (long double)xr[j];                         \
            const long double y1 = cx ? (long double)xi[j] : 0.0L;             \
            rr += cr[i * 2 + j] * x1 - ci[i * 2 + j] * y1;                     \
            ri += cr[i * 2 + j] * y1 + ci[i * 2 + j] * x1;                     \
            cabs += ((cr[i * 2 + j] < 0.0L) ? -cr[i * 2 + j]                   \
                                              : cr[i * 2 + j]) +               \
                    ((ci[i * 2 + j] < 0.0L) ? -ci[i * 2 + j]                   \
                                              : ci[i * 2 + j]);                \
        }                                                                      \
        if (rr < 0.0L) rr = -rr;                                               \
        if (ri < 0.0L) ri = -ri;                                               \
        babs = (long double)d_tests_abs_##S(_c->br[i]) +                       \
               (cx ? (long double)d_tests_abs_##S(_c->bi[i]) : 0.0L);          \
        bound = (4.0L * (long double)smini +                                   \
                 8.0L * (long double)EPS * cabs) * (long double)xnorm +        \
                8.0L * (long double)EPS * (long double)scale * babs;           \
        ok = ok && (rr + ri <= bound);                                         \
    }                                                                          \
    return ok;                                                                 \
}                                                                              \
                                                                               \
/* the reflector of (alpha, x), both forms: alike, and H (alpha, x) */         \
/* beta e1 to 4 epsilons of |(alpha, x)|, in units of TMIN          */         \
static bool                                                                    \
d_tests_guard_reflector_##S(T _alpha, const T* _x, size_t _count)              \
{                                                                              \
    T al = _alpha, al2 = _alpha, x[4], x2[4], tau, tau2;                       \
    long double v[5], w[5], dot = 0.0L, norm = 0.0L, err = 0.0L;               \
    const long double unit = 1.0L / (long double)TMIN;                         \
    bool ok;                                                                   \
    size_t i;                                                                  \
    memcpy(x, _x, sizeof(T) * _count);                                         \
    memcpy(x2, _x, sizeof(T) * _count);                                        \
    tau  = d_internal_decomp_##S##_reflector(&al, x, _count, 1);               \
    tau2 = d_internal_decomp_##S##_reflector_c(&al2, x2, _count, 1);           \
    ok = d_tests_same_##S(tau, tau2) && d_tests_same_##S(al, al2);             \
    for (i = 0; i < _count; ++i) ok = ok && d_tests_same_##S(x[i], x2[i]);     \
    w[0] = (long double)_alpha * unit;                                         \
    v[0] = 1.0L;                                                               \
    for (i = 0; i < _count; ++i)                                               \
    {                                                                          \
        w[i + 1] = (long double)_x[i] * unit;                                  \
        v[i + 1] = (long double)x[i];                                          \
    }                                                                          \
    for (i = 0; i <= _count; ++i)                                              \
    {                                                                          \
        dot  += v[i] * w[i];                                                   \
        norm += w[i] * w[i];                                                   \
    }                                                                          \
    for (i = 0; i <= _count; ++i)                                              \
    {                                                                          \
        const long double e = w[i] - (long double)tau * dot * v[i] -           \
                              ((i == 0) ? (long double)al * unit : 0.0L);      \
        err += e * e;                                                          \
    }                                                                          \
    return ok && (d_math_ld_sqrt(err) <=                                       \
                  4.0L * (long double)EPS * d_math_ld_sqrt(norm));             \
}                                                                              \
                                                                               \
/* the norms of _x, real and complex, both forms: alike, and, by    */         \
/* _kind, an infinity (1), a NaN (2), or _want in units of _unit, a  */        \
/* power of two, to 2 epsilons and the subnormals' spacing (0)       */        \
static bool                                                                    \
d_tests_guard_norm_##S(const T* _x, size_t _count, int _kind,                  \
                       long double _want, T _unit)                             \
{                                                                              \
    struct dj_cplx_##S cx[4], cy[4];                                           \
    T rn, rnc, cn, cnc, cm, cmc;                                               \
    long double got, gap, room;                                                \
    size_t i;                                                                  \
    for (i = 0; i < _count; ++i)                                               \
    {                                                                          \
        cx[i] = d_cplx_##S##_make(_x[i], (T)0);                                \
        cy[i] = d_cplx_##S##_make((T)0, _x[i]);                                \
    }                                                                          \
    rn  = d_internal_decomp_##S##_norm(_x, _count, 1);                         \
    rnc = d_internal_decomp_##S##_norm_c(_x, _count, 1);                       \
    cn  = d_internal_decomp_##S##_cnorm(cx, _count, 1);                        \
    cnc = d_internal_decomp_##S##_cnorm_c(cx, _count, 1);                      \
    cm  = d_internal_decomp_##S##_cnorm(cy, _count, 1);                        \
    cmc = d_internal_decomp_##S##_cnorm_c(cy, _count, 1);                      \
    if (!(d_tests_same_##S(rn, rnc) && d_tests_same_##S(rn, cn) &&             \
          d_tests_same_##S(rn, cnc) && d_tests_same_##S(rn, cm) &&             \
          d_tests_same_##S(rn, cmc)))                                          \
        return false;                                                          \
    if (_kind == 1) return rn == (T)HUGE_VAL;                                  \
    if (_kind == 2) return rn != rn;                                           \
    got  = (long double)rn / (long double)_unit;                               \
    gap  = got - _want;                                                        \
    room = 2.0L * (long double)EPS * _want;                                    \
    if ((long double)rn < (long double)TMIN)                                   \
        room = room + (long double)TMIN * (long double)EPS /                   \
                      (long double)_unit;                                      \
    return ((gap < 0.0L) ? -gap : gap) <= room;                                \
}                                                                              \
                                                                               \
/* _x _up / _down for powers of two _up and _down, in an order that */         \
/* passes no intermediate beyond the result's side of _x: exact      */        \
/* wherever the result is a normal number                            */        \
static long double                                                             \
d_tests_rescale_##S(long double _x, long double _up, long double _down)        \
{                                                                              \
    if (((_up >= 1.0L) && (_down < 1.0L)) ||                                   \
        ((_up < 1.0L) && (_down >= 1.0L)))                                     \
        return (_up >= 1.0L) ? (_x * _up) / _down : (_x / _down) * _up;        \
    return _x * (_up / _down);                                                 \
}                                                                              \
                                                                               \
/* the n-by-n _a balanced, both forms: alike; X = P D, the balancing */        \
/* undone on the identity, one entry in each row and column; rows    */        \
/* below ihi and columns above ilo triangular; and each entry of B   */        \
/* D^-1 P^T A P D's exactly where _exact, else, as dgebal lets an    */        \
/* entry that passes below the least normal on the way lose bits, to */        \
/* within its own magnitude and the subnormals' spacing              */        \
static bool                                                                    \
d_tests_guard_balance_##S(const T* _a, size_t _n, bool _exact)                 \
{                                                                              \
    T b[144], bc[144], xb[144], scale[12], scalec[12], dcol[12];               \
    size_t perm[12], permc[12], rowof[12], ilo, ihi, iloc, ihic, i, j;         \
    bool ok;                                                                   \
    memcpy(b, _a, sizeof(T) * _n * _n);                                        \
    memcpy(bc, _a, sizeof(T) * _n * _n);                                       \
    d_decomp_##S##_balance(b, _n, perm, scale, &ilo, &ihi);                    \
    d_decomp_##S##_balance_c(bc, _n, permc, scalec, &iloc, &ihic);             \
    ok = (ilo == iloc) && (ihi == ihic) && (ilo <= ihi) && (ihi < _n);         \
    for (i = 0; i < _n; ++i)                                                   \
        ok = ok && (perm[i] == permc[i]) &&                                    \
             d_tests_same_##S(scale[i], scalec[i]);                            \
    for (i = 0; i < _n * _n; ++i)                                              \
    {                                                                          \
        ok = ok && d_tests_same_##S(b[i], bc[i]);                              \
        xb[i] = ((i / _n) == (i % _n)) ? (T)1 : (T)0;                          \
    }                                                                          \
    d_decomp_##S##_balance_back(xb, _n, _n, perm, scale, ilo, ihi);            \
    for (j = 0; j < _n; ++j)                                                   \
    {                                                                          \
        size_t found = 0;                                                      \
        for (i = 0; i < _n; ++i)                                               \
            if (xb[i * _n + j] != (T)0)                                        \
            {                                                                  \
                ++found;                                                       \
                rowof[j] = i;                                                  \
                dcol[j]  = xb[i * _n + j];                                     \
            }                                                                  \
        ok = ok && (found == 1);                                               \
    }                                                                          \
    for (i = 0; ok && (i < _n); ++i)                                           \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            const long double e = d_tests_rescale_##S(                         \
                                      (long double)_a[rowof[i] * _n +          \
                                                      rowof[j]],               \
                                      (long double)dcol[j],                    \
                                      (long double)dcol[i]);                   \
            const long double g = (long double)b[i * _n + j] - e;              \
            const long double room = _exact ? 0.0L                             \
                : ((e < 0.0L) ? -e : e) +                                      \
                  (long double)TMIN * (long double)EPS;                        \
            ok = ok && (((g < 0.0L) ? -g : g) <= room);                        \
        }                                                                      \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j < i; ++j)                                                \
            if ((i > ihi) || (j < ilo)) ok = ok && (b[i * _n + j] == (T)0);    \
    return ok;                                                                 \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_guard_##S(void)                                                        \
{                                                                              \
    const T H = d_internal_math_##S##_pow2((EMAX) / 2 + 6);                    \
    const T G = d_internal_math_##S##_pow2((EMAX) / 2 - 8);                    \
    const T B = d_internal_math_##S##_pow2((EMAX) - 4);                        \
    const T tiny = TMIN * (T)16;                                               \
    size_t i, j, k;                                                            \
    bool ok;                                                                   \
                                                                               \
    /* dlaln2's paths: a pivot below smin, the solution's two scalings, */     \
    /* 1 by 1 and 2 by 2, real and complex; H / 2 a right-hand side    */      \
    /* where the matrix holds H                                         */     \
    {                                                                          \
        const T h2 = H / (T)2, m3 = (T)1e-3, s2 = (T)2 * TMIN;                 \
        const T s4 = (T)4 * TMIN;                                              \
        const struct dj_tests_solve_##S c[] = {                                \
            { 1, 1, m3, { 2, 0, 0, 0 }, { 1, 0 }, { 0, 0 }, 2, 0, false },     \
            { 1, 1, s2, { s4, 0, 0, 0 }, { 4, 0 }, { 0, 0 }, 0, 0, true },     \
            { 1, 2, m3, { 1, 0, 0, 0 }, { 1, 0 }, { 1, 0 }, 1, 0, false },     \
            { 1, 2, s2, { s4, 0, 0, 0 }, { 4, 0 }, { 1, 0 }, 0, 0, true },     \
            { 2, 1, m3, { 0, 0, 0, 0 }, { 1, 2 }, { 0, 0 }, 0, 0, false },     \
            { 2, 1, s4, { 0, 0, 0, 0 }, { 4, 1 }, { 0, 0 }, 0, 0, true },      \
            { 2, 2, m3, { 0, 0, 0, 0 }, { 1, 2 }, { 1, -1 }, 0, 0, false },    \
            { 2, 2, s4, { 0, 0, 0, 0 }, { 4, 1 }, { 1, 1 }, 0, 0, true },      \
            { 2, 1, m3, { 1, 1, 1, 1 }, { 1, 2 }, { 0, 0 }, 0, 0, false },     \
            { 2, 1, s2, { 1, 1, 1, 1 }, { 4, 1 }, { 0, 0 }, 0, 0, true },      \
            { 2, 1, 1, { H, H, H, H }, { 0, h2 }, { 0, 0 }, 0, 0, true },      \
            { 2, 2, m3, { 1, 1, 1, 1 }, { 1, 2 }, { 1, 0 }, 0, 0, true },      \
            { 2, 2, s2, { 1, 1, 1, 1 }, { 4, 1 }, { 1, 0 }, 0, 0, true },      \
            { 2, 2, 1, { H, H, H, H }, { 0, h2 }, { 0, h2 }, 0, 0, true },     \
            { 2, 2, m3, { 1, 2, -3, 1 }, { 1, 2 }, { 1, 0 }, 1, 2, false },    \
            { 2, 2, 1, { H, 0, 0, 1 }, { 0, h2 }, { 0, 0 }, 0, 0, true }       \
        };                                                                     \
        ok = true;                                                             \
        for (k = 0; k < sizeof(c) / sizeof(c[0]); ++k)                         \
            ok = ok && d_tests_guard_solve_##S(&c[k]);                         \
        D_TESTS_EXPECT(ok, #S " small_solve: smin's pivots and the "           \
                           "scalings, X to its residual");                     \
    }                                                                          \
                                                                               \
    /* dlanv2's paths: triangular either way, already standard, real, */       \
    /* complex, the sums scaled down and up, real after all; then     */       \
    /* near-defective blocks R [s 0; c s] R^T, R a rotation by a      */       \
    /* rational parameter, among which the rows swapped after the    */        \
    /* rotation                                                       */       \
    {                                                                          \
        const T p[][4] = {                                                     \
            { 1, 2, 0, 3 }, { 1, 0, 2, 3 }, { 1, 2, -3, 1 }, { 4, 1, 2, 3 },   \
            { 1, -2, 3, 4 }, { 0, -1, 0.5, 0.125 }, { 1, -2, 3, 0 },           \
            { 1, 1, 1, 1 }                                                     \
        };                                                                     \
        ok = true;                                                             \
        for (k = 0; k < sizeof(p) / sizeof(p[0]); ++k)                         \
        {                                                                      \
            const T s = (k == 5) ? B : (k == 6) ? tiny : (T)1;                 \
            const T e = (k == 7) ? EPS : (T)1;                                 \
            ok = ok && d_tests_guard_pair_##S(p[k][0] * s, p[k][1] * s * e,    \
                                              p[k][2] * s * e, p[k][3] * s);   \
        }                                                                      \
        for (k = 0; k < 20000; ++k)                                            \
        {                                                                      \
            const T u  = (T)(d_tests_random() / 4.0);                          \
            const T co = ((T)1 - u * u) / ((T)1 + u * u);                      \
            const T si = ((T)2 * u) / ((T)1 + u * u);                          \
            const T s0 = (T)(d_tests_random() / 4.0);                          \
            const T c0 = (T)(d_tests_random() / 4.0);                          \
            const T m00 = co * s0 - si * c0, m01 = -si * s0;                   \
            const T m10 = si * s0 + co * c0, m11 = co * s0;                    \
            ok = ok && d_tests_guard_pair_##S(m00 * co - m01 * si,             \
                                              m00 * si + m01 * co,             \
                                              m10 * co - m11 * si,             \
                                              m10 * si + m11 * co);            \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " standard_pair: every path, R^T A R the "       \
                           "block, standard, its values");                     \
    }                                                                          \
                                                                               \
    /* the scaled norms, real and complex: past the range, past the  */        \
    /* safe minimum, subnormal, zero, infinite, NaN                  */        \
    {                                                                          \
        const T big[4]  = { B * 4, B * 4, B * 4, 0 };                          \
        const T low[4]  = { TMIN, TMIN / 4, 0, 0 };                            \
        const T sub[2]  = { TMIN / 8, TMIN / 16 };                             \
        const T zero[2] = { 0, 0 };                                            \
        T inf[2], nan[2];                                                      \
        inf[0] = (T)HUGE_VAL;                                                  \
        inf[1] = (T)1;                                                         \
        nan[0] = (T)HUGE_VAL - (T)HUGE_VAL;                                    \
        nan[1] = (T)1;                                                         \
        ok = d_tests_guard_norm_##S(big, 4, 0, d_math_ld_sqrt(3.0L),           \
                                    B * 4) &&                                  \
             d_tests_guard_norm_##S(low, 4, 0,                                 \
                                    d_math_ld_sqrt(17.0L) / 4.0L, TMIN) &&     \
             d_tests_guard_norm_##S(sub, 2, 0,                                 \
                                    d_math_ld_sqrt(5.0L) / 16.0L, TMIN) &&     \
             d_tests_guard_norm_##S(zero, 2, 0, 0.0L, (T)1) &&                 \
             d_tests_guard_norm_##S(inf, 2, 1, 0.0L, (T)1) &&                  \
             d_tests_guard_norm_##S(nan, 2, 2, 0.0L, (T)1);                    \
        D_TESTS_EXPECT(ok, #S " norm, cnorm: scaled at the range's ends, "     \
                           "both forms");                                      \
    }                                                                          \
                                                                               \
    /* the reflector of entries below the safe minimum: lifted, found, */      \
    /* lowered                                                          */     \
    {                                                                          \
        const T x1[3] = { TMIN, TMIN, 0 };                                     \
        const T x2[3] = { TMIN * EPS, 0, 0 };                                  \
        ok = d_tests_guard_reflector_##S(TMIN / (T)2, x1, 3) &&                \
             d_tests_guard_reflector_##S((T)0, x2, 3);                         \
        D_TESTS_EXPECT(ok, #S " reflector: below the safe minimum, H x "       \
                           "beta e1");                                         \
    }                                                                          \
                                                                               \
    /* hypot of a NaN; the parts of order 0 */                                 \
    {                                                                          \
        T a[1] = { 0 }, scale[1], w[1];                                        \
        struct dj_cplx_##S v[1];                                               \
        size_t perm[1], ilo = 7, ihi = 7, iloc = 7, ihic = 7;                  \
        const T nan = (T)HUGE_VAL - (T)HUGE_VAL;                               \
        ok = (d_internal_decomp_##S##_hypot(nan, (T)1) !=                      \
              d_internal_decomp_##S##_hypot(nan, (T)1)) &&                     \
             (d_internal_decomp_##S##_hypot_c(nan, (T)1) !=                    \
              d_internal_decomp_##S##_hypot_c(nan, (T)1));                     \
        D_TESTS_EXPECT(ok, #S " hypot: a NaN's, a NaN");                       \
        d_decomp_##S##_balance(a, 0, perm, scale, &ilo, &ihi);                 \
        d_decomp_##S##_balance_c(a, 0, perm, scale, &iloc, &ihic);             \
        d_decomp_##S##_balance_back(a, 0, 0, perm, scale, 0, 0);               \
        d_decomp_##S##_schur_vectors(a, 0, a, w);                              \
        d_decomp_##S##_schur_vectors_c(a, 0, a, w);                            \
        ok = (ilo == 0) && (ihi == 0) && (iloc == 0) && (ihic == 0) &&         \
             d_decomp_##S##_schur(a, 0, 0, 0, a, v, 10u, (T)0) &&              \
             d_decomp_##S##_schur_c(a, 0, 0, 0, a, v, 10u, (T)0) &&            \
             (a[0] == (T)0);                                                   \
        D_TESTS_EXPECT(ok, #S " balance, balance_back, schur, "                \
                           "schur_vectors: order 0, nothing to do");           \
    }                                                                          \
                                                                               \
    /* a subnormal matrix: lifted before its scaling; its values */            \
    /* TMIN / 8 times 1 and 3, its vectors the unscaled matrix's */            \
    {                                                                          \
        const T m[4] = { 2, 1, 1, 2 };                                         \
        T a[4];                                                                \
        struct dj_cplx_##S v[2], x[4], v1[2], x1[4];                           \
        bool conv1 = false, conv = false;                                      \
        ok = d_tests_eigen_both_##S(m, 2, v1, x1, &conv1, 1000u);              \
        for (i = 0; i < 4; ++i) a[i] = m[i] * (TMIN / (T)8);                   \
        ok = ok && d_tests_eigen_both_##S(a, 2, v, x, &conv, 1000u) &&         \
             conv && conv1;                                                    \
        for (i = 0; i < 4; ++i) ok = ok && d_tests_csame_##S(x[i], x1[i]);     \
        for (k = 0; k < 2; ++k)                                                \
        {                                                                      \
            const long double want = (k == 0) ? 1.0L : 3.0L;                   \
            const long double got  = (long double)v[k].part[0] /               \
                                     ((long double)TMIN / 8.0L);               \
            ok = ok && (((got - want < 0.0L) ? want - got : got - want) <=     \
                        64.0L * (long double)EPS * want) &&                    \
                 (v[k].part[1] == (T)0);                                       \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " eigen: a subnormal matrix's values and "       \
                           "vectors");                                         \
    }                                                                          \
                                                                               \
    /* Hessenberg matrices of 1, -1 and 0 found to need the exceptional */     \
    /* shifts at the bottom, after twenty steps without a split, and   */      \
    /* the rarer standard blocks                                        */     \
    {                                                                          \
        static const signed char hard[][49] = {                                \
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 1, 0, 0,      \
              -1, -1, 0, 0, 1, 0, 1, -1, 0, 0, 0, 0, -1, 0, 1, -1, 0, 0, 0,    \
              0, 0, 1, 1, 0, 0, 0, 0, 0, 0, -1 },                              \
            { 1, 1, 1, 0, 0, 1, 0, 1, 1, -1, 0, 1, 0, 0, 0, -1, 0, 1, 0, 1,    \
              -1, 0, 0, -1, 0, -1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,      \
              1, -1, 0, 0, 0, 0, 0, 0, 1, 1 },                                 \
            { 0, -1, 0, -1, 0, -1, 1, -1, 0, -1, 0, 0, 0, 0, -1, 1 },          \
            { -1, 0, 1, 1, 0, 1, 1, 0, 0, -1, 1, 0, -1, -1, 0, 0, 1, 1, 0,     \
              0, 1, 0, 0, 1, 1, -1, 0, 0, 0, 0, 0, 1, 1, -1, 0, 0, 0, 0, 0,    \
              0, 0, -1, 0, 0, 0, 0, 0, 1, -1 },                                \
            { 0, -1, 1, 0, -1, -1, 1, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1,    \
              0, 0, 0, 0, 0, 0 },                                              \
            { 0, 0, 0, 0, 1, 1, 1, 0, -1, -1, 0, -1, 1, 0, -1, 0, 0, 0, -1,    \
              0, 0, 0, 0, -1, 1 },                                             \
            { 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, -1, 0, 0, 1, 0 },               \
            { -1, 1, 0, -1, 1, 0, -1, 0, -1, -1, 1, 0, 1, 1, 0, -1, -1, -1,    \
              0, -1, 1, 0, 0, -1, 0, -1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,      \
              0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 1 },                          \
            { 0, 1, 0, -1, 1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 1,      \
              0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },          \
            { 1, -1, 0, -1, 0, 1, 0, -1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, -1,     \
              0, 0, 0, 0, 1, 0 },                                              \
            { 0, 1, 0, 1, 0, 0, 1, -1, 0, 1, 1, 0, 0, -1, -1, 0, -1, -1, 0,    \
              0, -1, 0, 0, 1, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, -1, 1 },          \
            { 1, 0, 0, -1, 0, 1, 0, 1, 0 }                                     \
        };                                                                     \
        static const unsigned char order[] = {                                 \
            7, 7, 4, 7, 5, 5, 4, 7, 6, 5, 6, 3                                 \
        };                                                                     \
        T h[49];                                                               \
        ok = true;                                                             \
        for (k = 0; k < sizeof(order); ++k)                                    \
        {                                                                      \
            for (i = 0; i < (size_t)order[k] * order[k]; ++i)                  \
                h[i] = (T)hard[k][i];                                          \
            ok = ok && d_tests_guard_schur_##S(h, order[k]);                   \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " schur: the exceptional shifts at the "         \
                           "bottom, Z T Z^T H");                               \
    }                                                                          \
                                                                               \
    /* quasi-triangular T for the vectors, of entries G = 2^(EMAX/2-8): */     \
    /* a Jordan chain, whose vector's last entry underflows through    */      \
    /* the scalings; a real vector through a block of two; two equal   */      \
    /* pairs; four equal pairs, the last's two entries underflowing    */      \
    {                                                                          \
        T t[144];                                                              \
        const size_t ns[4] = { 6, 4, 5, 8 };                                   \
        ok = true;                                                             \
        for (k = 0; k < 4; ++k)                                                \
        {                                                                      \
            const size_t n = ns[k];                                            \
            for (i = 0; i < n * n; ++i) t[i] = (T)0;                           \
            if (k == 0)                                                        \
                for (i = 0; i < n; ++i)                                        \
                {                                                              \
                    t[i * n + i] = (T)1;                                       \
                    if (i + 1 < n) t[i * n + i + 1] = G;                       \
                }                                                              \
            else                                                               \
            {                                                                  \
                /* a first row of G, then blocks [1 2; -1/2 1] coupled */      \
                /* by G above the diagonal                              */     \
                t[0] = (T)0.5;                                                 \
                for (j = 1; j < n; ++j) t[j] = G;                              \
                for (i = 1; i + 1 < n; i += 2)                                 \
                {                                                              \
                    t[i * n + i] = t[(i + 1) * n + i + 1] = (T)1;              \
                    t[i * n + i + 1] = (T)2;                                   \
                    t[(i + 1) * n + i] = (T)-0.5;                              \
                    for (j = i + 2; j < n; ++j)                                \
                        t[i * n + j] = t[(i + 1) * n + j] = G;                 \
                }                                                              \
                if ((n % 2) == 0) t[(n - 1) * n + n - 1] = (T)1;               \
            }                                                                  \
            ok = ok && d_tests_guard_vectors_##S(t, n);                        \
        }                                                                      \
        /* and random ones: a block of two anywhere, G or 1 above it, */       \
        /* diagonals equal to rounding or apart                       */       \
        for (k = 0; k < 2000; ++k)                                             \
        {                                                                      \
            const size_t n = 3 + (k % 4), p = (k / 4) % (n - 1);               \
            const int kind = (int)((k / 16) % 8);                              \
            const T im = (T)d_tests_random() *                                 \
                         ((kind & 1) ? EPS : (T)0.25);                         \
            T bb = (T)0.25 + d_tests_abs_##S((T)d_tests_random());             \
            for (i = 0; i < n; ++i)                                            \
                for (j = 0; j < n; ++j)                                        \
                {                                                              \
                    T x = (T)0;                                                \
                    if (j > i)                                                 \
                        x = (T)d_tests_random() * ((kind & 2) ? G : (T)1);     \
                    if (j == i)                                                \
                        x = (kind & 4) ? (T)1 + (T)d_tests_random() * EPS      \
                                       : (T)d_tests_random();                  \
                    t[i * n + j] = x;                                          \
                }                                                              \
            t[p * n + p + 1] = bb;                                             \
            t[(p + 1) * n + p] = -(im * im) / bb;                              \
            if (t[(p + 1) * n + p] == (T)0) t[(p + 1) * n + p] = -TMIN;        \
            t[(p + 1) * n + p + 1] = t[p * n + p];                             \
            ok = ok && d_tests_guard_vectors_##S(t, n);                        \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " schur_vectors: the scalings, and a vector "    \
                           "underflowing, eigenvectors still");                \
    }                                                                          \
                                                                               \
    /* balancing matrices of entries from 2^-(EMAX-4) to 2^(EMAX-4), */        \
    /* a third of them zero: the norms' and the factors' guards; and  */       \
    /* of entries from 2^-(EMAX/8) to 2^(EMAX/8), where nothing        */      \
    /* passes below the least normal, exactly                          */      \
    {                                                                          \
        T a[36];                                                               \
        int range;                                                             \
        for (range = 0; range < 2; ++range)                                    \
        {                                                                      \
            const double span = (range == 0) ? (double)((EMAX) - 4)            \
                                             : (double)((EMAX) / 8);           \
            ok = true;                                                         \
            for (k = 0; k < 2000; ++k)                                         \
            {                                                                  \
                const size_t n = 2 + (k % 5);                                  \
                for (i = 0; i < n * n; ++i)                                    \
                {                                                              \
                    const double r = d_tests_random() / 4.0;                   \
                    const int e = (int)(d_tests_random() / 4.0 * span);        \
                    a[i] = (r < -0.3) ? (T)0                                   \
                         : (T)r * d_internal_math_##S##_pow2(e);               \
                }                                                              \
                ok = ok && d_tests_guard_balance_##S(a, n, range == 1);        \
            }                                                                  \
            D_TESTS_EXPECT(ok, (range == 0)                                    \
                ? #S " balance: entries across the range, D^-1 P^T A P D "     \
                     "but for entries passing below the least normal"          \
                : #S " balance: entries across 2^(EMAX/4), D^-1 P^T A P D "    \
                     "exactly");                                               \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* the Jacobi tangent past 2^(EMAX/2), at a threshold of 0: the */         \
    /* _c forms at run time, the plain forms' bits                   */        \
    {                                                                          \
        const T e = d_internal_math_##S##_pow2(-((EMAX) / 2) - 8);             \
        T w[4], w2[4], val[2], val2[2], vec[4], vec2[4];                       \
        T u[4], u2[4], sv[2], sv2[2], v[4], v2[4];                             \
        w[0] = w2[0] = (T)1;                                                   \
        w[1] = w2[1] = e;                                                      \
        w[2] = w2[2] = e;                                                      \
        w[3] = w2[3] = (T)2;                                                   \
        for (i = 0; i < 4; ++i)                                                \
            vec[i] = vec2[i] = v[i] = v2[i] = (T)((i % 3) == 0);               \
        d_decomp_##S##_sym_eigen(w, val, vec, 2, (T)0);                        \
        d_decomp_##S##_sym_eigen_c(w2, val2, vec2, 2, (T)0);                   \
        w[0] = w2[0] = (T)1;                                                   \
        w[1] = w2[1] = (T)0;                                                   \
        w[2] = w2[2] = e;                                                      \
        w[3] = w2[3] = (T)2;                                                   \
        d_decomp_##S##_svd(w, u, sv, v, 2, 2, (T)0);                           \
        d_decomp_##S##_svd_c(w2, u2, sv2, v2, 2, 2, (T)0);                     \
        ok = (d_tests_abs_##S(vec[1]) == e);                                   \
        for (i = 0; i < 4; ++i)                                                \
            ok = ok && d_tests_same_##S(vec[i], vec2[i]) &&                    \
                 d_tests_same_##S(u[i], u2[i]) &&                              \
                 d_tests_same_##S(v[i], v2[i]);                                \
        for (i = 0; i < 2; ++i)                                                \
            ok = ok && d_tests_same_##S(val[i], val2[i]) &&                    \
                 d_tests_same_##S(sv[i], sv2[i]);                              \
        D_TESTS_EXPECT(ok, #S " sym_eigen_c, svd_c: the tangent past "         \
                           "2^(EMAX/2), the plain forms' bits");               \
    }                                                                          \
                                                                               \
    /* earlier chunks' paths: an entry already zero, a column with  */         \
    /* nothing below it, a matrix not positive definite, and an    */          \
    /* eigenvector of a NaN                                         */         \
    {                                                                          \
        const T sym[9] = { 2, 0, 1, 0, 3, 0, 1, 0, 4 };                        \
        const T up[9]  = { 1, 2, 3, 0, 4, 5, 0, 0, 6 };                        \
        const T npd[4] = { 1, 2, 2, 1 };                                       \
        const T zc[9]  = { 0, 1, 2, 0, 3, 4, 0, 0, 5 };                        \
        T w[9], w2[9], val[3], val2[3], vec[9], vec2[9], q[9], r[9];           \
        T q2[9], r2[9], v[3], l[4];                                            \
        const long double s2 = d_math_ld_sqrt(2.0L);                           \
        bool found = true;                                                     \
        memcpy(w, sym, sizeof(w));                                             \
        memcpy(w2, sym, sizeof(w2));                                           \
        for (i = 0; i < 9; ++i) vec[i] = vec2[i] = (T)((i % 4) == 0);          \
        d_decomp_##S##_sym_eigen(w, val, vec, 3, (T)0);                        \
        d_decomp_##S##_sym_eigen_c(w2, val2, vec2, 3, (T)0);                   \
        ok = true;                                                             \
        for (i = 0; i < 3; ++i)                                                \
        {                                                                      \
            const long double want = 3.0L + ((long double)i - 1.0L) * s2;      \
            const long double gap = (long double)val[i] - want;                \
            ok = ok && d_tests_same_##S(val[i], val2[i]) &&                    \
                 (((gap < 0.0L) ? -gap : gap) <= 8.0L * (long double)EPS *     \
                                                  5.0L);                       \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " sym_eigen: an entry already zero");            \
        /* a first column zero: left as it is, Q R A */                        \
        memcpy(r, zc, sizeof(r));                                              \
        memcpy(r2, zc, sizeof(r2));                                            \
        for (i = 0; i < 9; ++i) q[i] = q2[i] = (T)((i % 4) == 0);              \
        d_decomp_##S##_qr(q, r, 3, 3, v);                                      \
        d_decomp_##S##_qr_c(q2, r2, 3, 3, v);                                  \
        ok = (r[0] == (T)0) && (r[3] == (T)0) && (r[6] == (T)0);               \
        for (i = 0; i < 9; ++i)                                                \
            ok = ok && d_tests_same_##S(r[i], r2[i]) &&                        \
                 d_tests_same_##S(q[i], q2[i]);                                \
        for (i = 0; i < 3; ++i)                                                \
            for (j = 0; j < 3; ++j)                                            \
            {                                                                  \
                long double e = -(long double)zc[i * 3 + j];                   \
                for (k = 0; k < 3; ++k)                                        \
                    e += (long double)q[i * 3 + k] *                           \
                         (long double)r[k * 3 + j];                            \
                ok = ok && (((e < 0.0L) ? -e : e) <=                           \
                            8.0L * (long double)EPS * 9.0L);                   \
            }                                                                  \
        D_TESTS_EXPECT(ok, #S " qr: a column already zero, Q R A");            \
        memcpy(w, up, sizeof(w));                                              \
        memcpy(w2, up, sizeof(w2));                                            \
        d_decomp_##S##_hessenberg(w, 3);                                       \
        d_decomp_##S##_hessenberg_c(w2, 3);                                    \
        ok = true;                                                             \
        for (i = 0; i < 9; ++i)                                                \
            ok = ok && (w[i] == up[i]) && (w2[i] == up[i]);                    \
        D_TESTS_EXPECT(ok, #S " hessenberg: nothing below, unchanged");        \
        ok = !d_decomp_##S##_cholesky(l, npd, 2) &&                            \
             !d_decomp_##S##_cholesky_c(l, npd, 2);                            \
        D_TESTS_EXPECT(ok, #S " cholesky: not positive definite, both "        \
                           "forms");                                           \
        memcpy(w, npd, sizeof(T) * 4);                                         \
        w[0] = (T)HUGE_VAL - (T)HUGE_VAL;                                      \
        ok = d_tests_eigenvector_##S(w, 2, (T)1, v, &found) && !found;         \
        D_TESTS_EXPECT(ok, #S " eigenvector: a NaN's, none");                  \
    }                                                                          \
}

D_TESTS_GUARD_FAMILY(float,       f,  FLT_EPSILON,  FLT_MIN,  FLT_MAX_EXP)
D_TESTS_GUARD_FAMILY(double,      d,  DBL_EPSILON,  DBL_MIN,  DBL_MAX_EXP)
D_TESTS_GUARD_FAMILY(long double, ld, LDBL_EPSILON, LDBL_MIN, LDBL_MAX_EXP)

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
    struct dj_cplx_##S v[3] = {};                                              \
    struct dj_cplx_##S x[9] = {};                                              \
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
    struct dj_cplx_##S v[3], x[9];                                             \
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

int
main(void)
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
    /* the guards, last: they draw from the generator too */
    d_tests_guard_f();
    d_tests_guard_d();
    d_tests_guard_ld();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_schur_constant_f();
    d_tests_schur_constant_d();
    d_tests_schur_constant_ld();
#endif

    printf("t_decomp: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
