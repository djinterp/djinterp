/*******************************************************************************
* djinterp [test]                                                       t_cmat.c
*
*   Conformance harness for c/math/cmat.h, all three families and both forms
* where a function has two. First, the systems of math_tests_cmat_refs.h --
* Gaussian-integer matrices whose solutions, inverses and determinants are
* exact, from rationals -- each family's determinant, solution and inverse
* within a bound of the exact ones, in epsilons times the order times the
* system's condition number; the solution each column LU's bits, and the
* determinant LU's; a singular matrix detected, its inverse and solution
* zero. Then the Hermitian matrices: the eigenvalues the references', in
* epsilons of |A|_F, ascending, the vectors unitary and A V = V L; both forms
* alike; and a real symmetric matrix's values decomp.h's sym_eigen's, bit for
* bit. Then the general path, every matrix of the references: the driver and
* its _c form alike; each eigenvalue within a bound of the reference, in
* epsilons times |A|_F times its condition number, ascending by real part
* then imaginary; each eigenvector unit, its largest entry real and
* positive, its residual within a bound of the balanced matrix's norm, as
* LAPACK's backward error is; 2^k A's results 2^k times A's, bit for bit;
* then the parts: balancing exact, the Hessenberg form zero below the
* subdiagonal with Q unitary and Q H Q^H the balanced matrix, the Schur
* form triangular with Z unitary and Z T Z^H the balanced matrix, the Schur
* vectors eigenvectors of it, each part's two forms alike; and matrices with
* known answers -- order 0, zero, the identity, a triangular matrix at
* either end of the range, no steps allowed, a NaN and an infinity. The
* largest measures are printed, family by family. From C++14, the
* eigenvalues, the Hermitian eigenvalues, the solution, the inverse and the
* determinant as constant expressions, for every family, with the bits they
* have at run time.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic-errors -Iinc             \
*        tests/djinterp/c/modules/math/t_cmat.c -o t_cmat -lm
*
*
* path:      /tests/djinterp/c/modules/math/t_cmat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.30
*                                                            revised: 2026.10.30
*******************************************************************************/
// std
#include <float.h>   // FLT_EPSILON ... LDBL_MIN, the families' limits
#include <math.h>    // HUGE_VAL
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/c/modules/math/cmat.h"     // unit under test
#include "../../../../../inc/djinterp/c/modules/math/decimal.h"  // d_math_decimal_to_ld
#include "../../../../../inc/djinterp/c/modules/math/decomp.h"   // d_decomp_*_sym_eigen
// tests
#include "./math_tests_cmat_refs.h"  // g_cmat_cases, g_cmat_systems


#define D_TESTS_COUNT(array)                                                   \
    (sizeof(array) / sizeof((array)[0]))

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

#define D_TESTS_EXPECT(ok, what)                                               \
    d_tests_expect((ok), (what), __LINE__)

// d_tests_random
//   a number in [-4, 4), from the harness's own generator.
static
double d_tests_random(void)
{
    g_state = (g_state * 1103515245u) + 12345u;

    return ((double)((g_state >> 4) % 2000003u) / 250000.0) - 4.0;
}

// d_tests_point
//   m 2^e, exactly, as the references give a part.
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


//==============================================================================
// 1.  THE FAMILIES
//==============================================================================

// D_TESTS_CMAT_FAMILY
//   stamps, for one family, the systems' checks (d_tests_systems_<S>), the
// Hermitian ones (d_tests_hermitian_<S>) and the general path's
// (d_tests_general_<S>), EMAX the family's largest exponent, with their
// bounds: SOLVE_BOUND in epsilons times
// the order times the condition number; HERM_BOUND in epsilons of |A|_F, n
// epsilons for the vectors' unitarity; VALUE_BOUND in epsilons times |A|_F
// times the eigenvalue's condition number; VECTOR_BOUND in epsilons of the
// balanced matrix's norm; SCHUR_BOUND in n epsilons, of it for the
// similarities. 2^SHIFT scales a matrix within the family's range; BIG is
// its largest power of two and SMALL its smallest normal number.
#define D_TESTS_CMAT_FAMILY(T, S, EPS, EMAX, SHIFT, BIG, SMALL,                \
                            SOLVE_BOUND, HERM_BOUND, VALUE_BOUND,              \
                            VECTOR_BOUND, SCHUR_BOUND)                         \
static long double g_worst_solve_##S  = 0.0L;                                  \
static long double g_worst_herm_##S   = 0.0L;                                  \
static long double g_worst_value_##S  = 0.0L;                                  \
static long double g_worst_vector_##S = 0.0L;                                  \
static long double g_worst_schur_##S  = 0.0L;                                  \
                                                                               \
static T                                                                       \
d_tests_abs_##S(T _x)                                                          \
{                                                                              \
    return (_x < (T)0) ? -_x : _x;                                             \
}                                                                              \
                                                                               \
static bool                                                                    \
d_tests_same_##S(T _x, T _y)                                                   \
{                                                                              \
    return (_x == _y) || ((_x != _x) && (_y != _y));                           \
}                                                                              \
                                                                               \
static bool                                                                    \
d_tests_csame_##S(struct d_cplx_##S _x, struct d_cplx_##S _y)                  \
{                                                                              \
    return d_tests_same_##S(_x.part[0], _y.part[0]) &&                         \
           d_tests_same_##S(_x.part[1], _y.part[1]);                           \
}                                                                              \
                                                                               \
/* the entry of the references at _k, in the family, exactly */                \
static struct d_cplx_##S                                                       \
d_tests_entry_##S(int _k)                                                      \
{                                                                              \
    const struct dj_tests_cmat_entry* e = &g_cmat_entries[_k];                  \
    return d_cplx_##S##_make((T)d_tests_point(e->rm, e->re),                   \
                             (T)d_tests_point(e->im, e->ie));                  \
}                                                                              \
                                                                               \
/* |_z - _w|, _w the reference at _k, in long double */                        \
static long double                                                             \
d_tests_distance_##S(struct d_cplx_##S _z, int _k)                             \
{                                                                              \
    const long double dr = (long double)_z.part[0] -                           \
                           d_tests_text(g_cmat_texts[_k].re);                  \
    const long double di = (long double)_z.part[1] -                           \
                           d_tests_text(g_cmat_texts[_k].im);                  \
    return d_math_ld_sqrt(dr * dr + di * di);                                  \
}                                                                              \
                                                                               \
/* the Frobenius norm of the _n complex entries of _a, in long double */       \
static long double                                                             \
d_tests_cfro_##S(const struct d_cplx_##S* _a, size_t _n)                       \
{                                                                              \
    long double s = 0.0L;                                                      \
    size_t i;                                                                  \
    for (i = 0; i < _n; ++i)                                                   \
        s += (long double)_a[i].part[0] * (long double)_a[i].part[0] +         \
             (long double)_a[i].part[1] * (long double)_a[i].part[1];          \
    return d_math_ld_sqrt(s);                                                  \
}                                                                              \
                                                                               \
/* the largest modulus of X^H X - I's entries, in long double */               \
static long double                                                             \
d_tests_unitarity_##S(const struct d_cplx_##S* _x, size_t _n)                  \
{                                                                              \
    long double worst = 0.0L;                                                  \
    size_t i, j, k;                                                            \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            long double sr = (i == j) ? -1.0L : 0.0L, si = 0.0L, m;            \
            for (k = 0; k < _n; ++k)                                           \
            {                                                                  \
                const long double ar = (long double)_x[k * _n + i].part[0];    \
                const long double ai = (long double)_x[k * _n + i].part[1];    \
                const long double br = (long double)_x[k * _n + j].part[0];    \
                const long double bi = (long double)_x[k * _n + j].part[1];    \
                sr += ar * br + ai * bi;                                       \
                si += ar * bi - ai * br;                                       \
            }                                                                  \
            m = d_math_ld_sqrt(sr * sr + si * si);                             \
            if (m > worst) worst = m;                                          \
        }                                                                      \
    return worst;                                                              \
}                                                                              \
                                                                               \
/* |X M X^H - B|_F, in long double */                                          \
static long double                                                             \
d_tests_similar_##S(const struct d_cplx_##S* _x,                               \
                    const struct d_cplx_##S* _m,                               \
                    const struct d_cplx_##S* _b, size_t _n)                    \
{                                                                              \
    long double xr[144], xi[144], s2 = 0.0L;                                   \
    size_t i, j, k;                                                            \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            long double sr = 0.0L, si = 0.0L;                                  \
            for (k = 0; k < _n; ++k)                                           \
            {                                                                  \
                const long double ar = (long double)_x[i * _n + k].part[0];    \
                const long double ai = (long double)_x[i * _n + k].part[1];    \
                const long double br = (long double)_m[k * _n + j].part[0];    \
                const long double bi = (long double)_m[k * _n + j].part[1];    \
                sr += ar * br - ai * bi;                                       \
                si += ar * bi + ai * br;                                       \
            }                                                                  \
            xr[i * _n + j] = sr;                                               \
            xi[i * _n + j] = si;                                               \
        }                                                                      \
    for (i = 0; i < _n; ++i)                                                   \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            long double sr = -(long double)_b[i * _n + j].part[0];             \
            long double si = -(long double)_b[i * _n + j].part[1];             \
            for (k = 0; k < _n; ++k)                                           \
            {                                                                  \
                const long double cr = (long double)_x[j * _n + k].part[0];    \
                const long double ci = -(long double)_x[j * _n + k].part[1];   \
                sr += xr[i * _n + k] * cr - xi[i * _n + k] * ci;               \
                si += xr[i * _n + k] * ci + xi[i * _n + k] * cr;               \
            }                                                                  \
            s2 += sr * sr + si * si;                                           \
        }                                                                      \
    return d_math_ld_sqrt(s2);                                                 \
}                                                                              \
                                                                               \
/* |A v - l v| / (eps |B|_F |v|), v column _col of the n-by-n _v, each */      \
/* row q divided by _s[q]: the balanced matrix B's residual            */      \
static long double                                                             \
d_tests_residual_##S(const struct d_cplx_##S* _a, size_t _n,                   \
                     const struct d_cplx_##S* _v, size_t _col,                 \
                     struct d_cplx_##S _l, const T* _s, long double _norm_b)   \
{                                                                              \
    long double r2 = 0.0L, v2 = 0.0L;                                          \
    const long double lr = (long double)_l.part[0];                            \
    const long double li = (long double)_l.part[1];                            \
    size_t q, j;                                                               \
    for (q = 0; q < _n; ++q)                                                   \
    {                                                                          \
        const long double vr = (long double)_v[q * _n + _col].part[0];         \
        const long double vi = (long double)_v[q * _n + _col].part[1];         \
        const long double s  = (long double)_s[q];                             \
        long double rr = -(lr * vr - li * vi);                                 \
        long double ri = -(lr * vi + li * vr);                                 \
        for (j = 0; j < _n; ++j)                                               \
        {                                                                      \
            const long double ar = (long double)_a[q * _n + j].part[0];        \
            const long double ai = (long double)_a[q * _n + j].part[1];        \
            const long double wr = (long double)_v[j * _n + _col].part[0];     \
            const long double wi = (long double)_v[j * _n + _col].part[1];     \
            rr += ar * wr - ai * wi;                                           \
            ri += ar * wi + ai * wr;                                           \
        }                                                                      \
        r2 += (rr * rr + ri * ri) / (s * s);                                   \
        v2 += (vr * vr + vi * vi) / (s * s);                                   \
    }                                                                          \
    if ((v2 == 0.0L) || (_norm_b == 0.0L)) return (r2 == 0.0L) ? 0.0L : 1e30L; \
    return d_math_ld_sqrt(r2) /                                                \
           ((long double)EPS * _norm_b * d_math_ld_sqrt(v2));                  \
}                                                                              \
                                                                               \
/* the systems: the determinant, the solution and the inverse against */       \
/* the exact ones, to SOLVE_BOUND n epsilons times the condition; the */       \
/* solution LU's bits; LU's determinant the determinant's             */       \
static void                                                                    \
d_tests_systems_##S(void)                                                      \
{                                                                              \
    const long double eps = (long double)EPS;                                  \
    size_t c, i, j;                                                            \
                                                                               \
    for (c = 0; c < D_TESTS_COUNT(g_cmat_systems); ++c)                        \
    {                                                                          \
        const struct dj_tests_cmat_system* sy = &g_cmat_systems[c];             \
        const size_t n = (size_t)sy->n;                                        \
        const long double cond = d_tests_text(sy->cond);                       \
        struct d_cplx_##S a[144], b[24], x[24], lu[144], scratch[144];         \
        struct d_cplx_##S inv[144], col[12], xc[12], det, det2;                \
        T sign = (T)0;                                                         \
        size_t perm[12];                                                       \
        long double e2 = 0.0L, r2 = 0.0L, worst = 0.0L, d;                     \
        bool ok, okinv, singular;                                              \
                                                                               \
        for (i = 0; i < n * n; ++i) a[i] = d_tests_entry_##S(sy->a + (int)i);  \
        for (i = 0; i < 2 * n; ++i) b[i] = d_tests_entry_##S(sy->b + (int)i);  \
                                                                               \
        /* the determinant, by LU in both ways */                              \
        det = d_cmat_##S##_determinant(a, scratch, n);                         \
        memcpy(lu, a, sizeof(lu[0]) * n * n);                                  \
        singular = d_cmat_##S##_lu(lu, n, perm, &sign);                        \
        det2 = d_cmat_##S##_lu_det(lu, n, sign);                               \
        D_TESTS_EXPECT(!singular && d_tests_csame_##S(det, det2),              \
                       #S " determinant: LU's, bit for bit");                  \
        d = d_tests_distance_##S(det, sy->x + (int)(2 * n + n * n));           \
        {                                                                      \
            const long double dr =                                             \
                d_tests_text(g_cmat_texts[sy->x + (int)(2 * n + n * n)].re);   \
            const long double di =                                             \
                d_tests_text(g_cmat_texts[sy->x + (int)(2 * n + n * n)].im);   \
            d = d / (d_math_ld_sqrt(dr * dr + di * di) * (long double)n *      \
                     eps * cond);                                              \
        }                                                                      \
        if (d > worst) worst = d;                                              \
                                                                               \
        /* the solution, against the exact one and LU's */                     \
        ok = d_cmat_##S##_solve(x, scratch, a, b, n, 2);                       \
        for (j = 0; j < 2; ++j)                                                \
        {                                                                      \
            for (i = 0; i < n; ++i) col[i] = b[i * 2 + j];                     \
            d_cmat_##S##_lu_solve(lu, perm, n, col, xc);                       \
            for (i = 0; i < n; ++i)                                            \
                ok = ok && d_tests_csame_##S(xc[i], x[i * 2 + j]);             \
        }                                                                      \
        D_TESTS_EXPECT(ok, #S " solve: true, each column LU's, bit for bit");  \
        for (i = 0; i < 2 * n; ++i)                                            \
        {                                                                      \
            const long double dr = d_tests_text(g_cmat_texts[sy->x +           \
                                                             (int)i].re);      \
            const long double di = d_tests_text(g_cmat_texts[sy->x +           \
                                                             (int)i].im);      \
            d = d_tests_distance_##S(x[i], sy->x + (int)i);                    \
            e2 += d * d;                                                       \
            r2 += dr * dr + di * di;                                           \
        }                                                                      \
        d = d_math_ld_sqrt(e2) /                                               \
            (d_math_ld_sqrt(r2) * (long double)n * eps * cond);                \
        if (d > worst) worst = d;                                              \
                                                                               \
        /* the inverse */                                                      \
        okinv = d_cmat_##S##_inverse(inv, scratch, a, n);                      \
        e2 = 0.0L;                                                             \
        r2 = 0.0L;                                                             \
        for (i = 0; i < n * n; ++i)                                            \
        {                                                                      \
            const int k = sy->x + (int)(2 * n + i);                            \
            const long double dr = d_tests_text(g_cmat_texts[k].re);           \
            const long double di = d_tests_text(g_cmat_texts[k].im);           \
            d = d_tests_distance_##S(inv[i], k);                               \
            e2 += d * d;                                                       \
            r2 += dr * dr + di * di;                                           \
        }                                                                      \
        d = d_math_ld_sqrt(e2) /                                               \
            (d_math_ld_sqrt(r2) * (long double)n * eps * cond);                \
        if (d > worst) worst = d;                                              \
        D_TESTS_EXPECT(okinv, #S " inverse: true");                            \
        if (worst > g_worst_solve_##S) g_worst_solve_##S = worst;              \
        D_TESTS_EXPECT(worst <= (long double)SOLVE_BOUND,                      \
                       #S " determinant, solve, inverse: the exact ones', "    \
                          "to the condition");                                 \
    }                                                                          \
                                                                               \
    /* a singular matrix whose elimination is exact: row 3 a quarter */        \
    /* of row 1 and half of row 2, its last pivot exactly 0           */       \
    {                                                                          \
        const double ent[9][2] = { { 4, 0 }, { 0, 2 }, { 8, 0 },               \
                                   { 2, 0 }, { 1, 1 }, { 2, 0 },               \
                                   { 2, 0 }, { 0.5, 1 }, { 3, 0 } };           \
        struct d_cplx_##S a[9], b[6], x[6], inv[9], scratch[9], det;           \
        T sign = (T)0;                                                         \
        size_t perm[3];                                                        \
        bool ok;                                                               \
        for (i = 0; i < 9; ++i)                                                \
            a[i] = d_cplx_##S##_make((T)ent[i][0], (T)ent[i][1]);              \
        for (i = 0; i < 6; ++i)                                                \
            b[i] = d_cplx_##S##_make((T)(int)i, (T)1);                         \
        det = d_cmat_##S##_determinant(a, scratch, 3);                         \
        ok = (det.part[0] == (T)0) && (det.part[1] == (T)0) &&                 \
             !d_cmat_##S##_inverse(inv, scratch, a, 3) &&                      \
             !d_cmat_##S##_solve(x, scratch, a, b, 3, 2);                      \
        for (i = 0; i < 9; ++i)                                                \
            ok = ok && (inv[i].part[0] == (T)0) && (inv[i].part[1] == (T)0);   \
        for (i = 0; i < 6; ++i)                                                \
            ok = ok && (x[i].part[0] == (T)0) && (x[i].part[1] == (T)0);       \
        memcpy(scratch, a, sizeof(a));                                         \
        ok = ok && d_cmat_##S##_lu(scratch, 3, perm, &sign);                   \
        D_TESTS_EXPECT(ok, #S " a singular matrix: determinant 0, no "         \
                           "inverse or solution, zeros, LU says so");          \
    }                                                                          \
                                                                               \
    return;                                                                    \
}                                                                              \
                                                                               \
/* the Hermitian matrices: values the references', to HERM_BOUND */            \
/* epsilons of |A|_F, ascending; vectors unitary, A V = V L; both  */          \
/* forms alike; a real symmetric matrix decomp.h's values, exactly */          \
static void                                                                    \
d_tests_hermitian_##S(void)                                                    \
{                                                                              \
    const T eps = EPS;                                                         \
    size_t c, i, j, k;                                                         \
                                                                               \
    for (c = 0; c < D_TESTS_COUNT(g_cmat_cases); ++c)                          \
    {                                                                          \
        const struct dj_tests_cmat_case* cs = &g_cmat_cases[c];                 \
        const size_t n = (size_t)cs->n;                                        \
        struct d_cplx_##S a[144], w[144], wc[144], vec[144], vecc[144];        \
        T val[12], valc[12], ones[12];                                         \
        long double norm, worst = 0.0L, u;                                     \
        bool ok = true;                                                        \
                                                                               \
        if (!cs->hermitian) continue;                                          \
        for (i = 0; i < n * n; ++i)                                            \
        {                                                                      \
            a[i] = d_tests_entry_##S(cs->first + (int)i);                      \
            w[i] = wc[i] = a[i];                                               \
            vec[i] = vecc[i] = d_cplx_##S##_make(                              \
                ((i / n) == (i % n)) ? (T)1 : (T)0, (T)0);                     \
        }                                                                      \
        norm = d_tests_cfro_##S(a, n * n);                                     \
        d_cmat_##S##_hermitian_eigen(w, val, vec, n, eps * (T)norm);           \
        d_cmat_##S##_hermitian_eigen_c(wc, valc, vecc, n, eps * (T)norm);      \
        for (i = 0; i < n; ++i) ok = ok && d_tests_same_##S(val[i], valc[i]);  \
        for (i = 0; i < n * n; ++i)                                            \
            ok = ok && d_tests_csame_##S(vec[i], vecc[i]);                     \
        D_TESTS_EXPECT(ok, #S " hermitian_eigen_c is hermitian_eigen, bit "    \
                           "for bit");                                         \
        if (norm == 0.0L) norm = 1.0L;                                         \
        ok = true;                                                             \
        for (k = 0; k < n; ++k)                                                \
        {                                                                      \
            const long double d =                                              \
                (long double)val[k] -                                          \
                d_tests_text(g_cmat_texts[cs->value + (int)k].re);             \
            const long double r = ((d < 0.0L) ? -d : d) /                      \
                                  ((long double)EPS * norm);                   \
            if (r > worst) worst = r;                                          \
            if (k > 0) ok = ok && (val[k - 1] <= val[k]);                      \
        }                                                                      \
        u = d_tests_unitarity_##S(vec, n) / ((long double)EPS *                \
                                             (long double)n);                  \
        if (u > worst) worst = u;                                              \
        for (i = 0; i < n; ++i) ones[i] = (T)1;                                \
        for (k = 0; k < n; ++k)                                                \
        {                                                                      \
            const long double r = d_tests_residual_##S(                        \
                a, n, vec, k, d_cplx_##S##_make(val[k], (T)0), ones,           \
                norm);                                                         \
            if (r > worst) worst = r;                                          \
        }                                                                      \
        if (worst > g_worst_herm_##S) g_worst_herm_##S = worst;                \
        D_TESTS_EXPECT(ok && (worst <= (long double)HERM_BOUND),               \
                       #S " hermitian_eigen: the references', ascending, "     \
                          "unitary, A V = V L");                               \
    }                                                                          \
                                                                               \
    /* the tangent past 2^(EMAX / 2), where theta^2 would overflow, */         \
    /* at a threshold of 0: |a_pq| / diff, the rotation made         */        \
    {                                                                          \
        const T e = d_internal_math_##S##_pow2(-(EMAX / 2) - 8);               \
        struct d_cplx_##S w[4], vec[4];                                        \
        T val[2];                                                              \
        w[0] = d_cplx_##S##_make((T)1, (T)0);                                  \
        w[1] = d_cplx_##S##_make((T)0, e);                                     \
        w[2] = d_cplx_##S##_make((T)0, -e);                                    \
        w[3] = d_cplx_##S##_make((T)2, (T)0);                                  \
        for (i = 0; i < 4; ++i)                                                \
            vec[i] = d_cplx_##S##_make(((i % 3) == 0) ? (T)1 : (T)0, (T)0);    \
        d_cmat_##S##_hermitian_eigen(w, val, vec, 2, (T)0);                    \
        D_TESTS_EXPECT((val[0] == (T)1) && (val[1] == (T)2) &&                 \
                       (d_tests_abs_##S(vec[1].part[0]) +                      \
                        d_tests_abs_##S(vec[1].part[1]) == e) &&               \
                       (d_tests_abs_##S(vec[2].part[0]) +                      \
                        d_tests_abs_##S(vec[2].part[1]) == e),                 \
                       #S " hermitian_eigen: the tangent past 2^(EMAX/2), "    \
                          "|a_pq| / diff");                                    \
    }                                                                          \
                                                                               \
    /* a real symmetric matrix: decomp.h's values, bit for bit */              \
    for (c = 0; c < 200; ++c)                                                  \
    {                                                                          \
        const size_t n = 1 + (c % 8);                                          \
        T s[64], sw[64], sv[64], sval[8], val[8], valc[8], fro = (T)0;         \
        struct d_cplx_##S w[64], wc[64], vec[64], vecc[64];                    \
        bool ok = true;                                                        \
        for (i = 0; i < n; ++i)                                                \
            for (j = 0; j <= i; ++j)                                           \
                s[i * n + j] = s[j * n + i] = (T)d_tests_random();             \
        for (i = 0; i < n * n; ++i)                                            \
        {                                                                      \
            fro += s[i] * s[i];                                                \
            sw[i] = s[i];                                                      \
            sv[i] = ((i / n) == (i % n)) ? (T)1 : (T)0;                        \
            w[i] = wc[i] = d_cplx_##S##_make(s[i], (T)0);                      \
            vec[i] = vecc[i] = d_cplx_##S##_make(sv[i], (T)0);                 \
        }                                                                      \
        fro = d_math_##S##_sqrt(fro);                                          \
        d_decomp_##S##_sym_eigen_c(sw, sval, sv, n, eps * fro);                \
        d_cmat_##S##_hermitian_eigen(w, val, vec, n, eps * fro);               \
        d_cmat_##S##_hermitian_eigen_c(wc, valc, vecc, n, eps * fro);          \
        for (i = 0; i < n; ++i)                                                \
            ok = ok && d_tests_same_##S(val[i], sval[i]) &&                    \
                 d_tests_same_##S(valc[i], sval[i]);                           \
        D_TESTS_EXPECT(ok, #S " hermitian_eigen: a real symmetric matrix, "    \
                           "sym_eigen's values, bit for bit");                 \
    }                                                                          \
                                                                               \
    return;                                                                    \
}                                                                              \
                                                                               \
/* the general path, both forms, on a copy of _a; whether they agree */        \
static bool                                                                    \
d_tests_eigen_both_##S(const struct d_cplx_##S* _a, size_t _n,                 \
                       struct d_cplx_##S* _v, struct d_cplx_##S* _x,           \
                       bool* _ok, size_t _max_iter)                            \
{                                                                              \
    struct d_cplx_##S w1[144], w2[144], k1[156], k2[156], v2[12], x2[144];     \
    T r1[24], r2[24];                                                          \
    size_t p1[12], p2[12], i;                                                  \
    bool ok2, same;                                                            \
    memcpy(w1, _a, sizeof(w1[0]) * _n * _n);                                   \
    memcpy(w2, _a, sizeof(w2[0]) * _n * _n);                                   \
    *_ok = d_cmat_##S##_eigen(w1, _n, _v, _x, k1, r1, p1, _max_iter, (T)0);    \
    ok2 = d_cmat_##S##_eigen_c(w2, _n, v2, x2, k2, r2, p2, _max_iter, (T)0);   \
    same = (*_ok == ok2);                                                      \
    for (i = 0; i < _n; ++i) same = same && d_tests_csame_##S(_v[i], v2[i]);   \
    for (i = 0; i < _n * _n; ++i)                                              \
        same = same && d_tests_csame_##S(_x[i], x2[i]);                        \
    return same;                                                               \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_general_case_##S(const struct dj_tests_cmat_case* _c)                   \
{                                                                              \
    const size_t n = (size_t)_c->n;                                            \
    const long double eps = (long double)EPS;                                  \
    struct d_cplx_##S a[144], b[144], bc[144], xb[144], h[144], hc[144];       \
    struct d_cplx_##S q[144], qc[144], t[144], tc[144], z[144], zc[144];       \
    struct d_cplx_##S vr[144], vrc[144], sa[144], v[12], x[144], v3[12];       \
    struct d_cplx_##S x3[144], sval[12], svalc[12], xs[12], xsc[12];           \
    T sv[12], dcol[12], scale[12], scalec[12], ones[12], nm[12], nmc[12];      \
    size_t perm[12], permc[12], rowof[12], ilo = 0, ihi = 0, iloc, ihic;       \
    long double norm, norm_b, worst = 0.0L;                                    \
    bool used[12], conv = false, conv3 = false, ok, okc;                       \
    size_t i, j, k, m;                                                         \
                                                                               \
    D_TESTS_EXPECT(n <= 12, #S " eigen: the case's order at most 12");         \
    if (n > 12) return;                                                        \
    for (i = 0; i < n * n; ++i) a[i] = d_tests_entry_##S(_c->first + (int)i);  \
    norm = d_tests_cfro_##S(a, n * n);                                         \
    if (norm == 0.0L) norm = 1.0L;                                             \
                                                                               \
    /* the driver, both forms */                                               \
    D_TESTS_EXPECT(d_tests_eigen_both_##S(a, n, v, x, &conv, 1000u),           \
                   #S " eigen_c is eigen, bit for bit");                       \
    D_TESTS_EXPECT(conv, #S " eigen: converged");                              \
    ok = true;                                                                 \
    for (k = 1; k < n; ++k)                                                    \
        ok = ok && ((v[k - 1].part[0] < v[k].part[0]) ||                       \
                    ((v[k - 1].part[0] == v[k].part[0]) &&                     \
                     (v[k - 1].part[1] <= v[k].part[1])));                     \
    D_TESTS_EXPECT(ok, #S " eigen: ascending, by real part then imaginary");   \
                                                                               \
    /* each eigenvalue against the nearest reference not yet taken */          \
    for (k = 0; k < n; ++k) used[k] = false;                                   \
    for (k = 0; k < n; ++k)                                                    \
    {                                                                          \
        long double best = -1.0L, cond = 1.0L;                                 \
        size_t pick = 0;                                                       \
        for (m = 0; m < n; ++m)                                                \
        {                                                                      \
            long double d;                                                     \
            if (used[m]) continue;                                             \
            d = d_tests_distance_##S(v[k], _c->value + (int)m);                \
            if ((best < 0.0L) || (d < best))                                   \
            {                                                                  \
                best = d;                                                      \
                pick = m;                                                      \
                cond = d_tests_text(g_cmat_texts[_c->value + (int)m].cond);    \
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
    /* balancing undone on the identity                            */          \
    memcpy(b, a, sizeof(a[0]) * n * n);                                        \
    memcpy(bc, a, sizeof(a[0]) * n * n);                                       \
    d_cmat_##S##_balance(b, n, perm, scale, &ilo, &ihi);                       \
    d_cmat_##S##_balance_c(bc, n, permc, scalec, &iloc, &ihic);                \
    ok = (ilo == iloc) && (ihi == ihic);                                       \
    for (i = 0; i < n; ++i)                                                    \
        ok = ok && (perm[i] == permc[i]) &&                                    \
             d_tests_same_##S(scale[i], scalec[i]);                            \
    for (i = 0; i < n * n; ++i) ok = ok && d_tests_csame_##S(b[i], bc[i]);     \
    D_TESTS_EXPECT(ok, #S " balance_c is balance, bit for bit");               \
    for (i = 0; i < n * n; ++i)                                                \
        xb[i] = d_cplx_##S##_make(((i / n) == (i % n)) ? (T)1 : (T)0, (T)0);   \
    d_cmat_##S##_balance_back(xb, n, n, perm, scale, ilo, ihi);                \
    ok = (ilo <= ihi) && (ihi < n);                                            \
    for (j = 0; j < n; ++j)                                                    \
    {                                                                          \
        size_t found = 0;                                                      \
        for (i = 0; i < n; ++i)                                                \
            if ((xb[i * n + j].part[0] != (T)0) ||                             \
                (xb[i * n + j].part[1] != (T)0))                               \
            {                                                                  \
                ++found;                                                       \
                rowof[j] = i;                                                  \
                dcol[j]  = xb[i * n + j].part[0];                              \
                sv[i]    = xb[i * n + j].part[0];                              \
                ok = ok && (xb[i * n + j].part[1] == (T)0);                    \
            }                                                                  \
        ok = ok && (found == 1);                                               \
    }                                                                          \
    for (i = 0; ok && (i < n); ++i)                                            \
        for (j = 0; j < n; ++j)                                                \
        {                                                                      \
            const struct d_cplx_##S e = a[rowof[i] * n + rowof[j]];            \
            ok = ok && (b[i * n + j].part[0] ==                                \
                        (e.part[0] * dcol[j]) / dcol[i]) &&                    \
                 (b[i * n + j].part[1] == (e.part[1] * dcol[j]) / dcol[i]);    \
        }                                                                      \
    for (i = 0; i < n; ++i)                                                    \
        for (j = 0; j < i; ++j)                                                \
            if ((i > ihi) || (j < ilo))                                        \
                ok = ok && (b[i * n + j].part[0] == (T)0) &&                   \
                     (b[i * n + j].part[1] == (T)0);                           \
    D_TESTS_EXPECT(ok, #S " balance: D^-1 P^T A P D exactly, isolated rows "   \
                       "and columns triangular");                              \
    norm_b = d_tests_cfro_##S(b, n * n);                                       \
                                                                               \
    /* each eigenvector: unit, its largest entry real and positive, */         \
    /* its residual the balanced matrix's few epsilons              */         \
    worst = 0.0L;                                                              \
    ok = true;                                                                 \
    for (k = 0; k < n; ++k)                                                    \
    {                                                                          \
        long double n2 = 0.0L, big = 0.0L, res;                                \
        bool top = false;                                                      \
        for (i = 0; i < n; ++i)                                                \
        {                                                                      \
            const long double re = (long double)x[i * n + k].part[0];          \
            const long double im = (long double)x[i * n + k].part[1];          \
            n2 += re * re + im * im;                                           \
            if (re * re + im * im > big) big = re * re + im * im;              \
        }                                                                      \
        for (i = 0; i < n; ++i)                                                \
            top = top || ((x[i * n + k].part[1] == (T)0) &&                    \
                          (x[i * n + k].part[0] > (T)0) &&                     \
                          ((long double)x[i * n + k].part[0] *                 \
                           (long double)x[i * n + k].part[0] >=                \
                           big * (1.0L - (long double)(8 * n) * eps)));        \
        ok = ok && top &&                                                      \
             (d_tests_abs_##S((T)(n2 - 1.0L)) <= (T)(4 * n) * EPS);            \
        res = d_tests_residual_##S(a, n, x, k, v[k], sv, norm_b);              \
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
        for (i = 0; i < n * n; ++i)                                            \
            sa[i] = d_cplx_##S##_make(a[i].part[0] * p2, a[i].part[1] * p2);   \
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
    /* the Hessenberg form of the balanced matrix, both forms */               \
    memcpy(h, b, sizeof(b[0]) * n * n);                                        \
    memcpy(hc, b, sizeof(b[0]) * n * n);                                       \
    for (i = 0; i < n * n; ++i)                                                \
        q[i] = qc[i] = d_cplx_##S##_make(((i / n) == (i % n)) ? (T)1 : (T)0,   \
                                         (T)0);                                \
    d_cmat_##S##_hessenberg_q(h, n, ilo, ihi, q);                              \
    d_cmat_##S##_hessenberg_q_c(hc, n, ilo, ihi, qc);                          \
    ok = true;                                                                 \
    for (i = 0; i < n * n; ++i)                                                \
        ok = ok && d_tests_csame_##S(h[i], hc[i]) &&                           \
             d_tests_csame_##S(q[i], qc[i]);                                   \
    D_TESTS_EXPECT(ok, #S " hessenberg_q_c is hessenberg_q, bit for bit");     \
    ok = true;                                                                 \
    for (i = 0; i < n; ++i)                                                    \
        for (j = 0; j + 1 < i; ++j)                                            \
            ok = ok && (h[i * n + j].part[0] == (T)0) &&                       \
                 (h[i * n + j].part[1] == (T)0);                               \
    worst = d_tests_unitarity_##S(q, n) / (eps * (long double)n);              \
    if (d_tests_similar_##S(q, h, b, n) / (eps * (long double)n * norm_b) >    \
        worst)                                                                 \
        worst = d_tests_similar_##S(q, h, b, n) /                              \
                (eps * (long double)n * norm_b);                               \
    if (worst > g_worst_schur_##S) g_worst_schur_##S = worst;                  \
    D_TESTS_EXPECT(ok && (worst <= (long double)SCHUR_BOUND),                  \
                   #S " hessenberg_q: zero below, Q unitary, Q H Q^H B");      \
                                                                               \
    /* the Schur form, both forms: triangular, its diagonal the values */      \
    memcpy(t, h, sizeof(h[0]) * n * n);                                        \
    memcpy(tc, h, sizeof(h[0]) * n * n);                                       \
    memcpy(z, q, sizeof(q[0]) * n * n);                                        \
    memcpy(zc, q, sizeof(q[0]) * n * n);                                       \
    ok  = d_cmat_##S##_schur(t, n, ilo, ihi, z, sval, 1000u, (T)0);            \
    okc = d_cmat_##S##_schur_c(tc, n, ilo, ihi, zc, svalc, 1000u, (T)0);       \
    okc = (ok == okc);                                                         \
    for (i = 0; i < n * n; ++i)                                                \
        okc = okc && d_tests_csame_##S(t[i], tc[i]) &&                         \
              d_tests_csame_##S(z[i], zc[i]);                                  \
    for (i = 0; i < n; ++i) okc = okc && d_tests_csame_##S(sval[i], svalc[i]); \
    D_TESTS_EXPECT(okc, #S " schur_c is schur, bit for bit");                  \
    for (i = 0; i < n; ++i)                                                    \
    {                                                                          \
        for (j = 0; j < i; ++j)                                                \
            ok = ok && (t[i * n + j].part[0] == (T)0) &&                       \
                 (t[i * n + j].part[1] == (T)0);                               \
        ok = ok && d_tests_csame_##S(sval[i], t[i * n + i]);                   \
    }                                                                          \
    worst = d_tests_unitarity_##S(z, n) / (eps * (long double)n);              \
    if (d_tests_similar_##S(z, t, b, n) / (eps * (long double)n * norm_b) >    \
        worst)                                                                 \
        worst = d_tests_similar_##S(z, t, b, n) /                              \
                (eps * (long double)n * norm_b);                               \
    if (worst > g_worst_schur_##S) g_worst_schur_##S = worst;                  \
    D_TESTS_EXPECT(ok && (worst <= (long double)SCHUR_BOUND),                  \
                   #S " schur: triangular, its diagonal the values, Z "        \
                      "unitary, Z T Z^H B");                                   \
                                                                               \
    /* the Schur vectors, both forms: eigenvectors of B */                     \
    memcpy(vr, z, sizeof(z[0]) * n * n);                                       \
    memcpy(vrc, z, sizeof(z[0]) * n * n);                                      \
    d_cmat_##S##_schur_vectors(t, n, vr, xs, nm);                              \
    d_cmat_##S##_schur_vectors_c(t, n, vrc, xsc, nmc);                         \
    ok = true;                                                                 \
    for (i = 0; i < n * n; ++i) ok = ok && d_tests_csame_##S(vr[i], vrc[i]);   \
    D_TESTS_EXPECT(ok, #S " schur_vectors_c is schur_vectors, bit for bit");   \
    worst = 0.0L;                                                              \
    for (i = 0; i < n; ++i) ones[i] = (T)1;                                    \
    for (k = 0; k < n; ++k)                                                    \
    {                                                                          \
        const long double res = d_tests_residual_##S(b, n, vr, k, sval[k],     \
                                                     ones, norm_b);            \
        if (res > worst) worst = res;                                          \
    }                                                                          \
    if (worst > g_worst_vector_##S) g_worst_vector_##S = worst;                \
    D_TESTS_EXPECT(worst <= (long double)VECTOR_BOUND,                         \
                   #S " schur_vectors: eigenvectors of Z T Z^H");              \
}                                                                              \
                                                                               \
static void                                                                    \
d_tests_general_##S(void)                                                      \
{                                                                              \
    size_t c, i;                                                               \
                                                                               \
    for (c = 0; c < D_TESTS_COUNT(g_cmat_cases); ++c)                          \
        d_tests_general_case_##S(&g_cmat_cases[c]);                            \
                                                                               \
    /* matrices with known answers */                                          \
    {                                                                          \
        struct d_cplx_##S zero3[9], id3[9], cyc3[9], big2[4], tiny2[4];        \
        struct d_cplx_##S bad[9], v[12], x[144];                               \
        bool conv = false, ok, nan = true;                                     \
        for (i = 0; i < 9; ++i)                                                \
        {                                                                      \
            zero3[i] = d_cplx_##S##_make((T)0, (T)0);                          \
            id3[i]   = d_cplx_##S##_make(((i % 4) == 0) ? (T)1 : (T)0, (T)0);  \
            cyc3[i]  = d_cplx_##S##_make(((i == 1) || (i == 5) || (i == 6))    \
                                         ? (T)1 : (T)0, (T)0);                 \
        }                                                                      \
        /* upper triangular: the diagonal, exactly, at either end of */        \
        /* the range                                                  */       \
        big2[0] = d_cplx_##S##_make(BIG, BIG);                                 \
        big2[1] = d_cplx_##S##_make(BIG, (T)0);                                \
        big2[2] = d_cplx_##S##_make((T)0, (T)0);                               \
        big2[3] = d_cplx_##S##_make(-BIG, (T)0);                               \
        for (i = 0; i < 4; ++i)                                                \
            tiny2[i] = d_cplx_##S##_make(big2[i].part[0] / BIG * SMALL,        \
                                         big2[i].part[1] / BIG * SMALL);       \
                                                                               \
        ok = d_tests_eigen_both_##S(zero3, 0, v, x, &conv, 1000u);             \
        D_TESTS_EXPECT(ok && conv, #S " eigen: order 0, nothing to find");     \
        ok = d_tests_eigen_both_##S(zero3, 3, v, x, &conv, 1000u) && conv;     \
        for (i = 0; i < 3; ++i)                                                \
            ok = ok && (v[i].part[0] == (T)0) && (v[i].part[1] == (T)0);       \
        for (i = 0; i < 9; ++i) ok = ok && d_tests_csame_##S(x[i], id3[i]);    \
        D_TESTS_EXPECT(ok, #S " eigen: the zero matrix, the identity's "       \
                           "columns");                                         \
        ok = d_tests_eigen_both_##S(id3, 3, v, x, &conv, 1000u) && conv;       \
        for (i = 0; i < 3; ++i)                                                \
            ok = ok && (v[i].part[0] == (T)1) && (v[i].part[1] == (T)0);       \
        for (i = 0; i < 9; ++i) ok = ok && d_tests_csame_##S(x[i], id3[i]);    \
        D_TESTS_EXPECT(ok, #S " eigen: the identity, exactly");                \
        ok = d_tests_eigen_both_##S(big2, 2, v, x, &conv, 1000u) && conv &&    \
             (v[0].part[0] == -BIG) && (v[0].part[1] == (T)0) &&               \
             (v[1].part[0] == BIG) && (v[1].part[1] == BIG);                   \
        D_TESTS_EXPECT(ok, #S " eigen: the largest power of two, exactly");    \
        ok = d_tests_eigen_both_##S(tiny2, 2, v, x, &conv, 1000u) && conv &&   \
             (v[0].part[0] == -SMALL) && (v[0].part[1] == (T)0) &&             \
             (v[1].part[0] == SMALL) && (v[1].part[1] == SMALL);               \
        D_TESTS_EXPECT(ok, #S " eigen: the smallest normal, exactly");         \
        ok = d_tests_eigen_both_##S(cyc3, 3, v, x, &conv, 0u) && !conv;        \
        for (i = 0; i < 9; ++i)                                                \
            nan = nan && (x[i].part[0] != x[i].part[0]) &&                     \
                  (x[i].part[1] != x[i].part[1]);                              \
        D_TESTS_EXPECT(ok && nan, #S " eigen: no steps allowed, not "          \
                                     "converged, NaN vectors");                \
        memcpy(bad, id3, sizeof(bad));                                         \
        bad[5].part[1] = d_internal_math_##S##_nan();                          \
        ok = d_tests_eigen_both_##S(bad, 3, v, x, &conv, 1000u) && !conv;      \
        for (i = 0; i < 3; ++i)                                                \
            ok = ok && (v[i].part[0] != v[i].part[0]) &&                       \
                 (v[i].part[1] != v[i].part[1]);                               \
        for (i = 0; i < 9; ++i) ok = ok && (x[i].part[0] != x[i].part[0]);     \
        D_TESTS_EXPECT(ok, #S " eigen: a NaN, every result NaN, flagged");     \
        bad[5].part[1] = -(T)HUGE_VAL;                                         \
        ok = d_tests_eigen_both_##S(bad, 3, v, x, &conv, 1000u) && !conv &&    \
             (v[1].part[0] != v[1].part[0]);                                   \
        D_TESTS_EXPECT(ok, #S " eigen: an infinity, every result NaN, "        \
                           "flagged");                                         \
    }                                                                          \
                                                                               \
    return;                                                                    \
}

D_TESTS_CMAT_FAMILY(float, f, FLT_EPSILON, FLT_MAX_EXP, 30,
                    d_internal_math_f_pow2(FLT_MAX_EXP - 1), FLT_MIN,
                    2, 32, 32, 32, 8)
D_TESTS_CMAT_FAMILY(double, d, DBL_EPSILON, DBL_MAX_EXP, 300,
                    d_internal_math_d_pow2(DBL_MAX_EXP - 1), DBL_MIN,
                    2, 32, 32, 32, 8)
D_TESTS_CMAT_FAMILY(long double, ld, LDBL_EPSILON, LDBL_MAX_EXP, 3000,
                    d_internal_math_ld_pow2(LDBL_MAX_EXP - 1), LDBL_MIN,
                    2, 32, 32, 32, 8)


//==============================================================================
// 2.  C++
//==============================================================================

#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )

// D_TESTS_CMAT_CONSTANT
//   stamps, for one family, d_cmat_<S>_eigen_c of a 2-by-2 matrix (one of
// order 3 takes clang past its million steps of constant evaluation),
// d_cmat_<S>_hermitian_eigen_c of a 2-by-2 one, and the solution, the
// inverse and the determinant of a 2-by-2 system, as constant expressions,
// one call for each part, and d_tests_constant_<S>, which compares them with
// the run time's, the matrices read through a volatile.
#define D_TESTS_CMAT_CONSTANT(T, S)                                            \
static constexpr struct d_cplx_##S                                             \
d_tests_constant_eigen_##S(size_t _k)                                          \
{                                                                              \
    struct d_cplx_##S a[4] = {                                                 \
        d_cplx_##S##_make(1, 2), d_cplx_##S##_make(0, -1),                     \
        d_cplx_##S##_make(2, 0), d_cplx_##S##_make(1, 1) };                    \
    struct d_cplx_##S v[2] = {};                                               \
    struct d_cplx_##S x[4] = {};                                               \
    struct d_cplx_##S work[6] = {};                                            \
    T rwork[4] = {};                                                           \
    size_t perm[2] = { 0, 0 };                                                 \
    (void)d_cmat_##S##_eigen_c(a, 2, v, x, work, rwork, perm, 1000u, (T)0);    \
    return (_k < 2) ? v[_k] : x[(_k - 2) * 2];                                 \
}                                                                              \
                                                                               \
static constexpr T                                                             \
d_tests_constant_hermitian_##S(size_t _k)                                      \
{                                                                              \
    struct d_cplx_##S a[4] = {                                                 \
        d_cplx_##S##_make(2, 0), d_cplx_##S##_make(1, -1),                     \
        d_cplx_##S##_make(1, 1), d_cplx_##S##_make(-1, 0) };                   \
    struct d_cplx_##S x[4] = {                                                 \
        d_cplx_##S##_make(1, 0), d_cplx_##S##_make(0, 0),                      \
        d_cplx_##S##_make(0, 0), d_cplx_##S##_make(1, 0) };                    \
    T v[2] = {};                                                               \
    d_cmat_##S##_hermitian_eigen_c(a, v, x, 2, (T)0);                          \
    return v[_k];                                                              \
}                                                                              \
                                                                               \
static constexpr struct d_cplx_##S                                             \
d_tests_constant_solve_##S(size_t _k)                                          \
{                                                                              \
    struct d_cplx_##S a[4] = {                                                 \
        d_cplx_##S##_make(1, 2), d_cplx_##S##_make(3, -1),                     \
        d_cplx_##S##_make(0, 1), d_cplx_##S##_make(2, 2) };                    \
    struct d_cplx_##S b[2] = {                                                 \
        d_cplx_##S##_make(1, 0), d_cplx_##S##_make(0, 1) };                    \
    struct d_cplx_##S x[2] = {};                                               \
    struct d_cplx_##S inv[4] = {};                                             \
    struct d_cplx_##S scratch[4] = {};                                         \
    (void)d_cmat_##S##_solve(x, scratch, a, b, 2, 1);                          \
    (void)d_cmat_##S##_inverse(inv, scratch, a, 2);                            \
    return (_k < 2) ? x[_k]                                                    \
                    : ((_k < 6) ? inv[_k - 2]                                  \
                                : d_cmat_##S##_determinant(a, scratch, 2));    \
}                                                                              \
                                                                               \
static constexpr struct d_cplx_##S g_constant_eigen_##S[4] =                   \
{                                                                              \
    d_tests_constant_eigen_##S(0), d_tests_constant_eigen_##S(1),              \
    d_tests_constant_eigen_##S(2), d_tests_constant_eigen_##S(3)               \
};                                                                             \
                                                                               \
static constexpr T g_constant_hermitian_##S[2] =                               \
{                                                                              \
    d_tests_constant_hermitian_##S(0), d_tests_constant_hermitian_##S(1)       \
};                                                                             \
                                                                               \
static constexpr struct d_cplx_##S g_constant_solve_##S[7] =                   \
{                                                                              \
    d_tests_constant_solve_##S(0), d_tests_constant_solve_##S(1),              \
    d_tests_constant_solve_##S(2), d_tests_constant_solve_##S(3),              \
    d_tests_constant_solve_##S(4), d_tests_constant_solve_##S(5),              \
    d_tests_constant_solve_##S(6)                                              \
};                                                                             \
                                                                               \
static void                                                                    \
d_tests_constant_##S(void)                                                     \
{                                                                              \
    volatile T one = (T)1;                                                     \
    struct d_cplx_##S a[4], h[4], x2[4], s[4], b[2], xs[2], inv[4], sc[4];     \
    struct d_cplx_##S v[2], x[4], work[6];                                     \
    T rwork[4], hv[2];                                                         \
    size_t perm[2], k;                                                         \
    bool ok = true;                                                            \
    const int ea[4][2] = { { 1, 2 }, { 0, -1 }, { 2, 0 }, { 1, 1 } };          \
    const int eh[4][2] = { { 2, 0 }, { 1, -1 }, { 1, 1 }, { -1, 0 } };         \
    const int es[4][2] = { { 1, 2 }, { 3, -1 }, { 0, 1 }, { 2, 2 } };          \
    for (k = 0; k < 4; ++k)                                                    \
    {                                                                          \
        a[k] = d_cplx_##S##_make((T)ea[k][0] * one, (T)ea[k][1] * one);        \
        h[k] = d_cplx_##S##_make((T)eh[k][0] * one, (T)eh[k][1] * one);        \
        s[k] = d_cplx_##S##_make((T)es[k][0] * one, (T)es[k][1] * one);        \
        x2[k] = d_cplx_##S##_make(((k % 3) == 0) ? one : (T)0, (T)0);          \
    }                                                                          \
    b[0] = d_cplx_##S##_make(one, (T)0);                                       \
    b[1] = d_cplx_##S##_make((T)0, one);                                       \
    (void)d_cmat_##S##_eigen(a, 2, v, x, work, rwork, perm, 1000u, (T)0);      \
    d_cmat_##S##_hermitian_eigen(h, hv, x2, 2, (T)0);                          \
    (void)d_cmat_##S##_solve(xs, sc, s, b, 2, 1);                              \
    (void)d_cmat_##S##_inverse(inv, sc, s, 2);                                 \
    for (k = 0; k < 2; ++k)                                                    \
        ok = ok && d_tests_csame_##S(g_constant_eigen_##S[k], v[k]) &&         \
             d_tests_csame_##S(g_constant_eigen_##S[k + 2], x[k * 2]) &&       \
             d_tests_same_##S(g_constant_hermitian_##S[k], hv[k]) &&           \
             d_tests_csame_##S(g_constant_solve_##S[k], xs[k]);                \
    for (k = 0; k < 4; ++k)                                                    \
        ok = ok && d_tests_csame_##S(g_constant_solve_##S[k + 2], inv[k]);     \
    ok = ok && d_tests_csame_##S(g_constant_solve_##S[6],                      \
                                 d_cmat_##S##_determinant(s, sc, 2));          \
    D_TESTS_EXPECT(ok, #S " eigen_c, hermitian_eigen_c, solve, inverse, "      \
                       "determinant: constant expressions, the run "           \
                       "time's bits");                                         \
}

D_TESTS_CMAT_CONSTANT(float,       f)
D_TESTS_CMAT_CONSTANT(double,      d)
D_TESTS_CMAT_CONSTANT(long double, ld)

#endif


int main(void)
{
    d_tests_systems_f();
    d_tests_systems_d();
    d_tests_systems_ld();
    d_tests_hermitian_f();
    d_tests_hermitian_d();
    d_tests_hermitian_ld();
    d_tests_general_f();
    d_tests_general_d();
    d_tests_general_ld();
#if ( defined(__cplusplus) &&                                                 \
      (__cplusplus >= 201402L) )
    d_tests_constant_f();
    d_tests_constant_d();
    d_tests_constant_ld();
#endif

    printf("  the worst, in epsilons, f d ld: systems %.3Lg %.3Lg %.3Lg; "
           "Hermitian %.3Lg %.3Lg %.3Lg; values %.3Lg %.3Lg %.3Lg; vectors "
           "%.3Lg %.3Lg %.3Lg; Schur %.3Lg %.3Lg %.3Lg\n",
           g_worst_solve_f, g_worst_solve_d, g_worst_solve_ld,
           g_worst_herm_f, g_worst_herm_d, g_worst_herm_ld,
           g_worst_value_f, g_worst_value_d, g_worst_value_ld,
           g_worst_vector_f, g_worst_vector_d, g_worst_vector_ld,
           g_worst_schur_f, g_worst_schur_d, g_worst_schur_ld);
    printf("t_cmat: %ld checks, %ld failures\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
