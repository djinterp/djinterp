"""gen_c_math.cmat - the text of cmat.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section)
from .decomp import (DECOMP_POP)


# -----------------------------------------------------------------------------
# driver
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# cmat.h
# -----------------------------------------------------------------------------
# Complex matrices (S269), on cplx.h's numbers: LU, determinant, inverse and
# solution, once per family; a Hermitian matrix's eigensystem by Jacobi's
# method; a general one's by LAPACK's zgeev path; what takes a root twice.

CMAT_FAMILY_HEAD = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Matrices are a caller's row-major arrays of cplx.h's complex numbers:
// element (i, j) of an r-by-c matrix at [i c + j], as LAPACK lays out its
// complex matrices but by rows. The real helpers - the scaled norms and
// hypot, the scaling by powers of two - are decomp.h's.

"""

CMAT_LU = r"""
// @SECTION@.1    Products, LU, determinant, inverse and solution
//------------------------------------------------------------------------------
// The product's entries are compensated, each part rounded once from Dot2's
// pair (vec.h's accumulator, S410), and multiply_fast is the formula as
// written. Gaussian elimination with partial pivoting, as LAPACK's zgetrf
// and zgesv: each column's pivot its entry of largest |re| + |im| at or
// below the diagonal (the first among equals), as LAPACK's izamax chooses
// it; every product and quotient cplx.h's, each part correctly rounded or
// within a step of it. solve refines the elimination's solution with extra
// precision, as mat.h's does, and solve_fast is the elimination alone.
// Needing no root, each comes once, a constant expression from C++14.

// d_internal_cmat_@S@_mul, d_internal_cmat_@S@_div
//   cplx.h's product and quotient, compiled once in a translation unit and
// called, where the kernels below would otherwise each take a copy of them
// at every call.
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_mul(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_cplx_@S@_mul(_z,
                          _w);
}

D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_div(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_cplx_@S@_div(_z,
                          _w);
}

// d_internal_cmat_@S@_abs1
//   |re| + |im| of _z: the magnitude LAPACK pivots by, needing no root.
D_MATH_FN @T@
d_internal_cmat_@S@_abs1(
    struct dj_cplx_@S@ _z
)
{
    const @T@ a = d_internal_math_@S@_abs(_z.part[0]);
    const @T@ b = d_internal_math_@S@_abs(_z.part[1]);

    return a + b;
}

// d_internal_cmat_@S@_is_zero
//   whether both parts of _z are zero.
D_MATH_FN bool
d_internal_cmat_@S@_is_zero(
    struct dj_cplx_@S@ _z
)
{
    return ( (_z.part[0] == (@T@)0) &&
             (_z.part[1] == (@T@)0) );
}

// d_internal_cmat_@S@_less_product
//   _a - _b _c, the product cplx.h's, then the difference part by part.
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_less_product(
    struct dj_cplx_@S@ _a,
    struct dj_cplx_@S@ _b,
    struct dj_cplx_@S@ _c
)
{
    return d_cplx_@S@_sub(_a,
                          d_internal_cmat_@S@_mul(_b,
                                         _c));
}

// d_internal_cmat_@S@_part_products
//   *_acc plus part _p of _a _x, negated where _minus: the real part's
// products re re and -im im, the imaginary part's re im and im re, each
// factor scaled first by 2^-_ka (_a's) or 2^-_kx (_x's) where either is not
// 0.
D_MATH_FN void
d_internal_cmat_@S@_part_products(
    struct dj_internal_vec_@S@_acc* _acc,
    int                             _p,
    bool                            _minus,
    struct dj_cplx_@S@              _a,
    struct dj_cplx_@S@              _x,
    int                             _ka,
    int                             _kx
)
{
    const bool scaled = ( (_ka != 0) ||
                          (_kx != 0) );
    const @T@  sign   = _minus ? (@T@)-1 : (@T@)1;
    @T@        ar     = _a.part[0];
    @T@        ai     = _a.part[1];
    @T@        xr     = sign * _x.part[0];
    @T@        xi     = sign * _x.part[1];

    // the factors scaled, where the operands leave the range
    if (scaled)
    {
        ar = d_internal_vec_@S@_scaled(ar, _ka);
        ai = d_internal_vec_@S@_scaled(ai, _ka);
        xr = d_internal_vec_@S@_scaled(xr, _kx);
        xi = d_internal_vec_@S@_scaled(xi, _kx);
    }

    // the real part: re re less im im
    if (_p == 0)
    {
        d_internal_vec_@S@_acc_add(_acc, ar, xr);
        d_internal_vec_@S@_acc_add(_acc, ai, -xi);
    }
    // the imaginary part: re im and im re
    else
    {
        d_internal_vec_@S@_acc_add(_acc, ar, xi);
        d_internal_vec_@S@_acc_add(_acc, ai, xr);
    }

    return;
}

// d_internal_cmat_@S@_naive
//   _c + sign sum_k _a[k _sa] (_x[k _sx] + _t[k]) as written, sign -1 where
// _minus and _t none but where _tailed: each product cplx.h's, then added or
// subtracted, in order - with _c 0 and no tail, d_cmat_@S@_multiply_fast's
// entry and the generic kernel's.
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_naive(
    struct dj_cplx_@S@        _c,
    bool                      _minus,
    const struct dj_cplx_@S@* _a,
    size_t                    _sa,
    const struct dj_cplx_@S@* _x,
    size_t                    _sx,
    const struct dj_cplx_@S@* _t,
    bool                      _tailed,
    size_t                    _n
)
{
    struct dj_cplx_@S@ acc = _c;
    size_t             k   = 0u;

    // each product, then the tail's
    for (k = 0u; k < _n; ++k)
    {
        const struct dj_cplx_@S@ p = d_internal_cmat_@S@_mul(_a[k * _sa],
                                                             _x[k * _sx]);

        acc = _minus ? d_cplx_@S@_sub(acc, p) : d_cplx_@S@_add(acc, p);

        if (_tailed)
        {
            const struct dj_cplx_@S@ q = d_internal_cmat_@S@_mul(_a[k * _sa],
                                                                 _t[k]);

            acc = _minus ? d_cplx_@S@_sub(acc, q) : d_cplx_@S@_add(acc, q);
        }
    }

    return acc;
}

// d_internal_cmat_@S@_survey
//   the reach of _z's parts, as vec.h's d_internal_vec_@S@_reach_of gives a
// value's, the larger of it and _reach; *_largest the larger of it and
// _z's parts' magnitudes.
D_MATH_FN int
d_internal_cmat_@S@_survey(
    struct dj_cplx_@S@ _z,
    int                _reach,
    @T@*               _largest
)
{
    int reach = _reach;
    int p     = 0;

    for (p = 0; p < 2; ++p)
    {
        const int r = d_internal_vec_@S@_reach_of(_z.part[p]);
        const @T@ a = d_internal_math_@S@_abs(_z.part[p]);

        reach = (r > reach) ? r : reach;

        // a NaN keeps no largest
        if (a > *_largest)
        {
            *_largest = a;
        }
    }

    return reach;
}

// d_internal_cmat_@S@_dot2
//   _c + sign sum_k _a[k _sa] (_x[k _sx] + _t[k]), sign -1 where _minus and
// _t none but where _tailed, each part rounded once from Dot2's pair
// (vec.h's accumulator): the real part's sum of re re and -im im products,
// the imaginary part's of re im and im re - as accurate as the sum in
// twice the precision, rounded once (S410). Where a part leaves the range
// where Dekker's products are exact, _a's parts are scaled by their largest's
// power, _x's and _t's by theirs, and _c with them, the sums scaled back, a
// part of _c far above every product its own; where every product is 0,
// each part of _c plus 0, +0 for either zero, as vec.h's dot gives it; and
// where a part is a NaN or an infinity, the formula as written
// (d_internal_cmat_@S@_naive).
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_internal_cmat_@S@_dot2(
    struct dj_cplx_@S@        _c,
    bool                      _minus,
    const struct dj_cplx_@S@* _a,
    size_t                    _sa,
    const struct dj_cplx_@S@* _x,
    size_t                    _sx,
    const struct dj_cplx_@S@* _t,
    bool                      _tailed,
    size_t                    _n
)
{
    struct dj_cplx_@S@ out   = _c;
    @T@                la    = (@T@)0;
    @T@                lx    = (@T@)0;
    @T@                lc    = (@T@)0;
    int                reach = 0;
    int                ka    = 0;
    int                kx    = 0;
    int                bits  = 0;
    size_t             left  = 4u * _n;
    size_t             k     = 0u;
    int                p     = 0;

    // each operand's reach and largest part
    for (k = 0u; k < _n; ++k)
    {
        reach = d_internal_cmat_@S@_survey(_a[k * _sa], reach, &la);
        reach = d_internal_cmat_@S@_survey(_x[k * _sx], reach, &lx);
        reach = _tailed ? d_internal_cmat_@S@_survey(_t[k], reach, &lx)
                        : reach;
    }

    reach = d_internal_cmat_@S@_survey(_c, reach, &lc);

    // a NaN or an infinity: the formula as written
    if (reach == 2)
    {
        return d_internal_cmat_@S@_naive(_c, _minus, _a, _sa, _x, _sx, _t,
                                         _tailed, _n);
    }

    // every product 0: _c's parts, +0 for either zero
    if ( (la == (@T@)0) ||
         (lx == (@T@)0) )
    {
        return d_cplx_@S@_make(_c.part[0] + (@T@)0,
                               _c.part[1] + (@T@)0);
    }

    // out of range: each operand by its largest part's power
    if (reach == 1)
    {
        ka = d_internal_vec_@S@_exponent_of(la);
        kx = d_internal_vec_@S@_exponent_of(lx);
    }

    // the products' sum lies below 2^(ka + kx + 2 + bits)
    while (left > 0u)
    {
        ++bits;
        left /= 2u;
    }

    for (p = 0; p < 2; ++p)
    {
        const @T@ c    = _c.part[p];
        const bool far =
            ( (reach == 1) &&
              (c != (@T@)0) &&
              (d_internal_vec_@S@_exponent_of(c) >
               (ka + kx + bits + @P@ + 3)) );
        struct dj_internal_vec_@S@_acc acc = d_internal_vec_@S@_acc_of(
            far ? (@T@)0 : d_internal_vec_@S@_scaled(c, ka + kx));

        // each product of the part's two kinds, the tail's with them
        for (k = 0u; k < _n; ++k)
        {
            d_internal_cmat_@S@_part_products(&acc, p, _minus, _a[k * _sa],
                                              _x[k * _sx], ka, kx);

            if (_tailed)
            {
                d_internal_cmat_@S@_part_products(&acc, p, _minus,
                                                  _a[k * _sa], _t[k], ka, kx);
            }
        }

        // a part of _c far above every product: its own
        out.part[p] = far ? c : d_internal_vec_@S@_acc_round(acc, ka + kx);
    }

    return out;
}

// d_cmat_@S@_multiply
//   _out (_rows x _cols) = _a (_rows x _inner) * _b (_inner x _cols), each
// entry's parts rounded once from Dot2's pair (d_internal_cmat_@S@_dot2);
// a vector is a matrix of one column. _out must not be an input.
D_MATH_FN_KERNEL void
d_cmat_@S@_multiply(
    struct dj_cplx_@S@*       _out,
    const struct dj_cplx_@S@* _a,
    const struct dj_cplx_@S@* _b,
    size_t                    _rows,
    size_t                    _inner,
    size_t                    _cols
)
{
    const struct dj_cplx_@S@ zero =
        d_cplx_@S@_make((@T@)0, (@T@)0);
    size_t                   i    = 0u;

    // each entry, row i of _a by column j of _b
    for (i = 0u; i < _rows; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _cols; ++j)
        {
            _out[(i * _cols) + j] =
                d_internal_cmat_@S@_dot2(zero, false, _a + (i * _inner), 1u,
                                         _b + j, _cols, _b, false, _inner);
        }
    }

    return;
}

// d_cmat_@S@_multiply_fast
//   the product as written: each entry's products cplx.h's, added in order
// of k (d_internal_cmat_@S@_naive).
D_MATH_FN_KERNEL void
d_cmat_@S@_multiply_fast(
    struct dj_cplx_@S@*       _out,
    const struct dj_cplx_@S@* _a,
    const struct dj_cplx_@S@* _b,
    size_t                    _rows,
    size_t                    _inner,
    size_t                    _cols
)
{
    const struct dj_cplx_@S@ zero =
        d_cplx_@S@_make((@T@)0, (@T@)0);
    size_t                   i    = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _cols; ++j)
        {
            _out[(i * _cols) + j] =
                d_internal_cmat_@S@_naive(zero, false, _a + (i * _inner), 1u,
                                          _b + j, _cols, _b, false, _inner);
        }
    }

    return;
}

// d_cmat_@S@_lu
//   factors the n-by-n _lu in place into P A = L U - L's multipliers below
// the diagonal, U on and above - choosing each column's pivot of largest
// |re| + |im| (the first, among equals). _perm, unless NULL, receives the row
// permutation, and *_sign its parity (+1 or -1). Returns whether some pivot
// column was zero, the matrix singular; that column is then skipped, U's
// diagonal zero there.
D_MATH_FN_KERNEL bool
d_cmat_@S@_lu(
    struct dj_cplx_@S@* _lu,
    size_t              _n,
    size_t*             _perm,
    @T@*                _sign
)
{
    bool   singular = false;
    size_t i        = 0u;
    size_t col      = 0u;

    *_sign = (@T@)1;

    if (_perm != NULL)
    {
        for (i = 0u; i < _n; ++i)
        {
            _perm[i] = i;
        }
    }

    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_cmat_@S@_abs1(_lu[(col * _n) + col]);
        size_t r     = 0u;

        // the largest |re| + |im| in the column, at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_cmat_@S@_abs1(_lu[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // a zero column: singular, and nothing to eliminate
        if (d_internal_cmat_@S@_is_zero(_lu[(pivot * _n) + col]))
        {
            singular = true;

            continue;
        }

        // the pivot row up
        if (pivot != col)
        {
            size_t c = 0u;

            for (c = 0u; c < _n; ++c)
            {
                const struct dj_cplx_@S@ tmp = _lu[(col * _n) + c];

                _lu[(col * _n) + c]   = _lu[(pivot * _n) + c];
                _lu[(pivot * _n) + c] = tmp;
            }

            if (_perm != NULL)
            {
                const size_t tmp = _perm[col];

                _perm[col]   = _perm[pivot];
                _perm[pivot] = tmp;
            }

            *_sign = -*_sign;
        }

        {
            const struct dj_cplx_@S@ pivot_value = _lu[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const struct dj_cplx_@S@ f =
                    d_internal_cmat_@S@_div(_lu[(r * _n) + col], pivot_value);
                size_t c = 0u;

                _lu[(r * _n) + col] = f;

                for (c = col + 1u; c < _n; ++c)
                {
                    _lu[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _lu[(r * _n) + c], f, _lu[(col * _n) + c]);
                }
            }
        }
    }

    return singular;
}

// d_cmat_@S@_lu_solve
//   solves A _x = _b from d_cmat_@S@_lu's factors: _b permuted, forward
// through L (unit diagonal), back through U. _x and _b must not overlap.
D_MATH_FN_KERNEL void
d_cmat_@S@_lu_solve(
    const struct dj_cplx_@S@* _lu,
    const size_t*             _perm,
    size_t                    _n,
    const struct dj_cplx_@S@* _b,
    struct dj_cplx_@S@*       _x
)
{
    size_t i = 0u;

    // forward: y into _x
    for (i = 0u; i < _n; ++i)
    {
        struct dj_cplx_@S@ s = _b[_perm[i]];
        size_t             k = 0u;

        for (k = 0u; k < i; ++k)
        {
            s = d_internal_cmat_@S@_less_product(s, _lu[(i * _n) + k],
                                                 _x[k]);
        }

        _x[i] = s;
    }

    // back: _x over y, last row first
    for (i = _n; i-- > 0u; )
    {
        struct dj_cplx_@S@ s = _x[i];
        size_t             k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            s = d_internal_cmat_@S@_less_product(s, _lu[(i * _n) + k],
                                                 _x[k]);
        }

        _x[i] = d_internal_cmat_@S@_div(s, _lu[(i * _n) + i]);
    }

    return;
}

// d_cmat_@S@_lu_det
//   the determinant from the factors: the parity times U's diagonal, in
// order.
D_MATH_FN struct dj_cplx_@S@
d_cmat_@S@_lu_det(
    const struct dj_cplx_@S@* _lu,
    size_t                    _n,
    @T@                       _sign
)
{
    struct dj_cplx_@S@ d = d_cplx_@S@_make(_sign, (@T@)0);
    size_t             i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        d = d_internal_cmat_@S@_mul(d, _lu[(i * _n) + i]);
    }

    return d;
}

// d_cmat_@S@_determinant
//   the determinant of the n-by-n _a, by its LU factors in _scratch (n by
// n): the parity times U's diagonal; 0 for a singular one.
D_MATH_FN_KERNEL struct dj_cplx_@S@
d_cmat_@S@_determinant(
    const struct dj_cplx_@S@* _a,
    struct dj_cplx_@S@*       _scratch,
    size_t                    _n
)
{
    @T@    sign = (@T@)1;
    size_t k    = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    // a singular matrix: 0
    if (d_cmat_@S@_lu(_scratch, _n, NULL, &sign))
    {
        return d_cplx_@S@_make((@T@)0, (@T@)0);
    }

    return d_cmat_@S@_lu_det(_scratch, _n, sign);
}

// d_cmat_@S@_inverse
//   _out = the inverse of the n-by-n _a, true; or, for a singular one, the
// zero matrix and false. _scratch holds n * n entries; neither it nor _out
// may be _a. Gauss-Jordan, as mat.h's d_mat_@S@_inverse: the pivot the
// first entry of largest |re| + |im| at or below the diagonal, its row
// divided by it, the others' multiples of it subtracted.
D_MATH_FN_KERNEL bool
d_cmat_@S@_inverse(
    struct dj_cplx_@S@*       _out,
    struct dj_cplx_@S@*       _scratch,
    const struct dj_cplx_@S@* _a,
    size_t                    _n
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    // the identity
    for (k = 0u; k < (_n * _n); ++k)
    {
        const bool diagonal = ((k / _n) == (k % _n));

        _out[k] = d_cplx_@S@_make(diagonal ? (@T@)1 : (@T@)0, (@T@)0);
    }

    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_cmat_@S@_abs1(_scratch[(col * _n) + col]);
        size_t r     = 0u;
        size_t c     = 0u;

        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_cmat_@S@_abs1(_scratch[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (d_internal_cmat_@S@_is_zero(_scratch[(pivot * _n) + col]))
        {
            for (k = 0u; k < (_n * _n); ++k)
            {
                _out[k] = d_cplx_@S@_make((@T@)0, (@T@)0);
            }

            return false;
        }

        // the pivot row up
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const struct dj_cplx_@S@ ta = _scratch[(col * _n) + c];
                const struct dj_cplx_@S@ ti = _out[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = ta;
                _out[(col * _n) + c]       = _out[(pivot * _n) + c];
                _out[(pivot * _n) + c]     = ti;
            }
        }

        // the pivot row over the pivot
        {
            const struct dj_cplx_@S@ d = _scratch[(col * _n) + col];

            for (c = 0u; c < _n; ++c)
            {
                _scratch[(col * _n) + c] =
                    d_internal_cmat_@S@_div(_scratch[(col * _n) + c], d);
                _out[(col * _n) + c] = d_internal_cmat_@S@_div(_out[(col * _n) + c],
                                                      d);
            }
        }

        // the other rows less their multiples of it
        for (r = 0u; r < _n; ++r)
        {
            if (r != col)
            {
                const struct dj_cplx_@S@ f = _scratch[(r * _n) + col];

                for (c = 0u; c < _n; ++c)
                {
                    _scratch[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _scratch[(r * _n) + c], f, _scratch[(col * _n) + c]);
                    _out[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _out[(r * _n) + c], f, _out[(col * _n) + c]);
                }
            }
        }
    }

    return true;
}

// d_cmat_@S@_solve_fast
//   _x = the solution X of A X = B, A n by n and B n by _m, true; or, for a
// singular A, the zero matrix and false. Gaussian elimination of [A | B]
// with partial pivoting, the pivot the first entry of largest |re| + |im| at
// or below the diagonal, then back-substitution, as LAPACK's zgesv: for each
// column of B the same bits as d_cmat_@S@_lu and d_cmat_@S@_lu_solve give.
// _scratch holds n * n entries; neither it nor _x may be _a or _b.
D_MATH_FN_KERNEL bool
d_cmat_@S@_solve_fast(
    struct dj_cplx_@S@*       _x,
    struct dj_cplx_@S@*       _scratch,
    const struct dj_cplx_@S@* _a,
    const struct dj_cplx_@S@* _b,
    size_t                    _n,
    size_t                    _m
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    for (k = 0u; k < (_n * _m); ++k)
    {
        _x[k] = _b[k];
    }

    // elimination: [A | B] to [U | Y]
    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_cmat_@S@_abs1(_scratch[(col * _n) + col]);
        size_t r     = 0u;
        size_t c     = 0u;

        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_cmat_@S@_abs1(_scratch[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (d_internal_cmat_@S@_is_zero(_scratch[(pivot * _n) + col]))
        {
            for (k = 0u; k < (_n * _m); ++k)
            {
                _x[k] = d_cplx_@S@_make((@T@)0, (@T@)0);
            }

            return false;
        }

        // the pivot row up, in A and in B
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const struct dj_cplx_@S@ t = _scratch[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = t;
            }

            for (c = 0u; c < _m; ++c)
            {
                const struct dj_cplx_@S@ t = _x[(col * _m) + c];

                _x[(col * _m) + c]   = _x[(pivot * _m) + c];
                _x[(pivot * _m) + c] = t;
            }
        }

        // the rows below less their multiples of it
        {
            const struct dj_cplx_@S@ pivot_value = _scratch[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const struct dj_cplx_@S@ f =
                    d_internal_cmat_@S@_div(_scratch[(r * _n) + col], pivot_value);

                for (c = col + 1u; c < _n; ++c)
                {
                    _scratch[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                        _scratch[(r * _n) + c], f, _scratch[(col * _n) + c]);
                }

                for (c = 0u; c < _m; ++c)
                {
                    _x[(r * _m) + c] = d_internal_cmat_@S@_less_product(
                        _x[(r * _m) + c], f, _x[(col * _m) + c]);
                }
            }
        }
    }

    // back-substitution, column by column of B, the last row first
    for (k = 0u; k < _m; ++k)
    {
        size_t i = 0u;

        for (i = _n; i-- > 0u; )
        {
            struct dj_cplx_@S@ s = _x[(i * _m) + k];
            size_t             j = 0u;

            for (j = i + 1u; j < _n; ++j)
            {
                s = d_internal_cmat_@S@_less_product(s, _scratch[(i * _n) + j],
                                                     _x[(j * _m) + k]);
            }

            _x[(i * _m) + k] = d_internal_cmat_@S@_div(s, _scratch[(i * _n) + i]);
        }
    }

    return true;
}
// d_internal_cmat_@S@_correction
//   _fix = the correction of the solution _x + _tail of A x = b (b's entries
// _sb apart): the residual b - A (x + tail), each part of each row by Dot2
// rounded once (d_internal_cmat_@S@_dot2), into _res, solved by A's factors
// (d_cmat_@S@_lu_solve). *_largest is the correction's largest part in
// magnitude, *_relative its largest relative to its part of x, at most 1 -
// 1 also where that part is 0.
D_MATH_FN_KERNEL void
d_internal_cmat_@S@_correction(
    struct dj_cplx_@S@*       _fix,
    struct dj_cplx_@S@*       _res,
    @T@*                      _largest,
    @T@*                      _relative,
    const struct dj_cplx_@S@* _x,
    const struct dj_cplx_@S@* _tail,
    const struct dj_cplx_@S@* _a,
    const struct dj_cplx_@S@* _b,
    size_t                    _sb,
    const struct dj_cplx_@S@* _lu,
    const size_t*             _perm,
    size_t                    _n
)
{
    size_t i = 0u;
    int    p = 0;

    *_largest  = (@T@)0;
    *_relative = (@T@)0;

    // the residual, row by row
    for (i = 0u; i < _n; ++i)
    {
        _res[i] = d_internal_cmat_@S@_dot2(_b[i * _sb], true, _a + (i * _n),
                                           1u, _x, 1u, _tail, true, _n);
    }

    d_cmat_@S@_lu_solve(_lu, _perm, _n, _res, _fix);

    // its sizes, part by part, whole and against x
    for (i = 0u; i < _n; ++i)
    {
        for (p = 0; p < 2; ++p)
        {
            const @T@ d = d_internal_math_@S@_abs(_fix[i].part[p]);
            const @T@ x = d_internal_math_@S@_abs(_x[i].part[p]);
            const @T@ r = (d >= x) ? (@T@)1 : (d / x);

            if (d > *_largest)
            {
                *_largest = d;
            }

            // a zero correction leaves its part as it is
            if ( (d != (@T@)0) &&
                 (r > *_relative) )
            {
                *_relative = r;
            }
        }
    }

    return;
}

// d_internal_cmat_@S@_refine
//   _x, one column of the solution (_n entries) as A's factors gave it,
// refined (d_cmat_@S@_solve); _tail, _fix and _res are _n entries each.
D_MATH_FN_KERNEL void
d_internal_cmat_@S@_refine(
    struct dj_cplx_@S@*       _x,
    struct dj_cplx_@S@*       _tail,
    struct dj_cplx_@S@*       _fix,
    struct dj_cplx_@S@*       _res,
    const struct dj_cplx_@S@* _a,
    const struct dj_cplx_@S@* _b,
    size_t                    _sb,
    const struct dj_cplx_@S@* _lu,
    const size_t*             _perm,
    size_t                    _n
)
{
    @T@    last_largest  = (@T@)0;
    @T@    last_relative = (@T@)0;
    size_t step          = 0u;
    size_t i             = 0u;
    int    p             = 0;

    for (i = 0u; i < _n; ++i)
    {
        _tail[i] = d_cplx_@S@_make((@T@)0, (@T@)0);
    }

    // each step: the correction, then whether it is worth adding
    for (step = 0u; step < (size_t)D_CFG_MATH_MAT_REFINE_STEPS; ++step)
    {
        @T@ largest  = (@T@)0;
        @T@ relative = (@T@)0;

        d_internal_cmat_@S@_correction(_fix, _res, &largest, &relative, _x,
                                       _tail, _a, _b, _sb, _lu, _perm, _n);

        // no progress: neither the correction nor its relative size halved
        if ( (step > 0u) &&
             (largest > (last_largest / (@T@)2)) &&
             (relative > (last_relative / (@T@)2)) )
        {
            break;
        }

        // the correction into the solution and its tail, part by part
        for (i = 0u; i < _n; ++i)
        {
            for (p = 0; p < 2; ++p)
            {
                d_internal_vec_@S@_add_pair(&_x[i].part[p], &_tail[i].part[p],
                                            _fix[i].part[p]);
            }
        }

        // converged: every part's correction below half its unit in the
        // last place
        if (relative <= (@EPS@ / (@T@)2))
        {
            break;
        }

        last_largest  = largest;
        last_relative = relative;
    }

    return;
}

// d_cmat_@S@_solve
//   _x = the solution X of A X = B, A n by n and B n by _m, true; or, for a
// singular A, the zero matrix and false. Each column first as
// d_cmat_@S@_solve_fast takes it, by A's factors (d_cmat_@S@_lu, kept in
// _scratch's first n * n entries, the permutation in _pivots, and
// d_cmat_@S@_lu_solve), then refined as mat.h's real solve refines (S406,
// S410): the residual's parts by Dot2, the solution carried with a tail,
// until every part's correction lies below half its unit in the last place,
// neither the largest correction nor the largest relative halves, or
// D_CFG_MATH_MAT_REFINE_STEPS steps have run. For an A far enough from
// singular, each part of the solution correctly rounded but where it lies
// within some cond(A) eps^2 of a midpoint. Where A, the column of B or the
// first solution has a NaN or an infinity, the column is solve_fast's.
// _scratch holds n * n + 4 n entries and _pivots n; neither may be an
// operand, nor _x _a or _b.
D_MATH_FN_KERNEL bool
d_cmat_@S@_solve(
    struct dj_cplx_@S@*       _x,
    struct dj_cplx_@S@*       _scratch,
    size_t*                   _pivots,
    const struct dj_cplx_@S@* _a,
    const struct dj_cplx_@S@* _b,
    size_t                    _n,
    size_t                    _m
)
{
    struct dj_cplx_@S@* const xc   = _scratch + (_n * _n);
    struct dj_cplx_@S@* const tail = xc + _n;
    struct dj_cplx_@S@* const fix  = tail + _n;
    struct dj_cplx_@S@* const res  = fix + _n;
    @T@                       sign = (@T@)1;
    @T@                       big  = (@T@)0;
    int                       a_ok = 0;
    size_t                    col  = 0u;
    size_t                    k    = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
        a_ok        = d_internal_cmat_@S@_survey(_a[k], a_ok, &big);
    }

    // singular: the zero matrix
    if (d_cmat_@S@_lu(_scratch, _n, _pivots, &sign))
    {
        for (k = 0u; k < (_n * _m); ++k)
        {
            _x[k] = d_cplx_@S@_make((@T@)0, (@T@)0);
        }

        return false;
    }

    // each column: solved by the factors, refined, written back
    for (col = 0u; col < _m; ++col)
    {
        int finite = a_ok;

        for (k = 0u; k < _n; ++k)
        {
            res[k] = _b[(k * _m) + col];
            finite = d_internal_cmat_@S@_survey(res[k], finite, &big);
        }

        d_cmat_@S@_lu_solve(_scratch, _pivots, _n, res, xc);

        for (k = 0u; k < _n; ++k)
        {
            finite = d_internal_cmat_@S@_survey(xc[k], finite, &big);
        }

        // finite throughout: refined
        if (finite != 2)
        {
            d_internal_cmat_@S@_refine(xc, tail, fix, res, _a, _b + col, _m,
                                       _scratch, _pivots, _n);
        }

        for (k = 0u; k < _n; ++k)
        {
            _x[(k * _m) + col] = xc[k];
        }
    }

    return true;
}

"""

CMAT_HERMITIAN_HEAD = r"""
// @SECTION@.2    Hermitian matrices
//------------------------------------------------------------------------------
// A Hermitian matrix's eigenvalues, real, and its eigenvectors, unitary, by
// the cyclic Jacobi method with complex rotations (S269): decomp.h's real
// symmetric method with each pair's phase taken out first. What takes a root
// comes twice: the _c form a constant expression from C++14, the plain one
// on the C library's root, the same bit for bit.

// gcc's -Warray-bounds is set aside for this section's kernels, and restored
// after it: where a caller's matrix is a constant gcc folds, its value ranges
// take loops below for reachable with indices they never take (gcc 13 at
// -O2, a 2-by-2 rotation in main), a false warning, and under -Werror an
// error. Every index below lies within the caller's n by n.
#if ( defined(__GNUC__) &&                                                    \
      !defined(__clang__) )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
#endif
"""

CMAT_HERMITIAN_FORMS = r"""

// d_cmat_@S@_hermitian_eigen@F@
//   the eigenvalues and eigenvectors of the Hermitian n-by-n _work by the
// cyclic Jacobi method with complex rotations, as decomp.h's
// d_decomp_@S@_sym_eigen@F@ finds a symmetric matrix's: sweeps of rotations,
// each annihilating an off-diagonal pair - its phase u = a_pq / |a_pq| taken
// out by the unitary diag(1, conj(u)) on q, then the real rotation of the
// pair [a_pp, |a_pq|; |a_pq|, a_qq] - until the off-diagonal part's norm is
// within _threshold, at most 100 sweeps. _work is consumed; _vectors, the
// n-by-n identity on entry, receives the eigenvectors as columns, a unitary
// matrix; _values the eigenvalues, the diagonal's real parts, ascending
// (the first smallest, among equals), the columns following. A real
// symmetric matrix gets d_decomp_@S@_sym_eigen@F@'s values bit for bit.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_hermitian_eigen@F@(
    struct dj_cplx_@S@* _work,
    @T@*                _values,
    struct dj_cplx_@S@* _vectors,
    size_t              _n,
    @T@                 _threshold
)
{
    size_t sweep = 0u;
    size_t p     = 0u;
    size_t q     = 0u;
    size_t i     = 0u;

    for (sweep = 0u; sweep < 100u; ++sweep)
    {
        @T@ off_sq = (@T@)0;

        for (p = 0u; p < _n; ++p)
        {
            for (q = p + 1u; q < _n; ++q)
            {
                const @T@ re  = _work[(p * _n) + q].part[0];
                const @T@ im  = _work[(p * _n) + q].part[1];
                const @T@ rr  = re * re;
                const @T@ ii  = im * im;

                off_sq = off_sq + rr;
                off_sq = off_sq + ii;
            }
        }

        // the off-diagonal part is small enough: converged
        if (d_math_@S@_sqrt@F@(off_sq) <= _threshold)
        {
            break;
        }

        for (p = 0u; p < _n; ++p)
        {
            for (q = p + 1u; q < _n; ++q)
            {
                const struct dj_cplx_@S@ apq = _work[(p * _n) + q];

                // already zero: nothing to annihilate
                if (d_internal_cmat_@S@_is_zero(apq))
                {
                    continue;
                }

                {
                    const @T@ r     = d_internal_decomp_@S@_hypot@F@(
                                          apq.part[0], apq.part[1]);
                    const struct dj_cplx_@S@ u =
                        d_cplx_@S@_make(apq.part[0] / r, apq.part[1] / r);
                    const struct dj_cplx_@S@ uc = d_cplx_@S@_conj(u);
                    const @T@ aqq   = _work[(q * _n) + q].part[0];
                    const @T@ app   = _work[(p * _n) + p].part[0];
                    const @T@ diff  = aqq - app;
                    const @T@ twoa  = (@T@)2 * r;
                    const @T@ half  = d_internal_math_@S@_pow2(@EMAX@ / 2);
                    const @T@ cut   = d_internal_math_@S@_abs(diff) / half;
                    const bool   far   = (cut >= twoa);
                    const @T@ theta = far ? (@T@)0 : (diff / twoa);
                    @T@       t     = (@T@)1;
                    @T@       c     = (@T@)0;
                    @T@       s     = (@T@)0;

                    // the smaller root of t^2 + 2 theta t - 1 = 0; past
                    // 2^(EMAX / 2), where theta^2 would overflow, 1 / (2
                    // theta), |a_pq| / diff, which it equals within a part
                    // in 2^EMAX
                    if ( (far) ||
                         (d_internal_math_@S@_abs(theta) >= half) )
                    {
                        t = r / diff;
                    }
                    else if (theta != (@T@)0)
                    {
                        const @T@ sgn  = (theta > (@T@)0) ? (@T@)1
                                                          : (@T@)-1;
                        const @T@ tt   = theta * theta;
                        const @T@ one  = tt + (@T@)1;
                        const @T@ root = d_math_@S@_sqrt@F@(one);
                        const @T@ mag  = d_internal_math_@S@_abs(theta);
                        const @T@ den  = mag + root;

                        t = sgn / den;
                    }

                    {
                        const @T@ tt   = t * t;
                        const @T@ one  = tt + (@T@)1;
                        const @T@ root = d_math_@S@_sqrt@F@(one);

                        c = (@T@)1 / root;
                        s = t * c;
                    }

                    // the columns p and q, column q turned by conj(u) first
                    for (i = 0u; i < _n; ++i)
                    {
                        const struct dj_cplx_@S@ wip = _work[(i * _n) + p];
                        const struct dj_cplx_@S@ wiq =
                            d_internal_cmat_@S@_mul(_work[(i * _n) + q], uc);
                        size_t k = 0u;

                        for (k = 0u; k < 2u; ++k)
                        {
                            const @T@ a1 = c * wip.part[k];
                            const @T@ a2 = s * wiq.part[k];
                            const @T@ b1 = s * wip.part[k];
                            const @T@ b2 = c * wiq.part[k];

                            _work[(i * _n) + p].part[k] = a1 - a2;
                            _work[(i * _n) + q].part[k] = b1 + b2;
                        }
                    }

                    // the rows p and q, row q turned by u first
                    for (i = 0u; i < _n; ++i)
                    {
                        const struct dj_cplx_@S@ wpj = _work[(p * _n) + i];
                        const struct dj_cplx_@S@ wqj =
                            d_internal_cmat_@S@_mul(u, _work[(q * _n) + i]);
                        size_t k = 0u;

                        for (k = 0u; k < 2u; ++k)
                        {
                            const @T@ a1 = c * wpj.part[k];
                            const @T@ a2 = s * wqj.part[k];
                            const @T@ b1 = s * wpj.part[k];
                            const @T@ b2 = c * wqj.part[k];

                            _work[(p * _n) + i].part[k] = a1 - a2;
                            _work[(q * _n) + i].part[k] = b1 + b2;
                        }
                    }

                    // V's columns, as A's
                    for (i = 0u; i < _n; ++i)
                    {
                        const struct dj_cplx_@S@ vip = _vectors[(i * _n) + p];
                        const struct dj_cplx_@S@ viq =
                            d_internal_cmat_@S@_mul(_vectors[(i * _n) + q], uc);
                        size_t k = 0u;

                        for (k = 0u; k < 2u; ++k)
                        {
                            const @T@ a1 = c * vip.part[k];
                            const @T@ a2 = s * viq.part[k];
                            const @T@ b1 = s * vip.part[k];
                            const @T@ b2 = c * viq.part[k];

                            _vectors[(i * _n) + p].part[k] = a1 - a2;
                            _vectors[(i * _n) + q].part[k] = b1 + b2;
                        }
                    }
                }
            }
        }
    }

    for (i = 0u; i < _n; ++i)
    {
        _values[i] = _work[(i * _n) + i].part[0];
    }

    // ascending, the vectors' columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;
        size_t k   = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            if (_values[k] < _values[sel])
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const @T@ tmp = _values[i];

            _values[i]   = _values[sel];
            _values[sel] = tmp;

            for (k = 0u; k < _n; ++k)
            {
                const struct dj_cplx_@S@ t = _vectors[(k * _n) + i];

                _vectors[(k * _n) + i]   = _vectors[(k * _n) + sel];
                _vectors[(k * _n) + sel] = t;
            }
        }
    }

    return;
}

"""

CMAT_GENERAL_HEAD = r"""
// @SECTION@.3    The eigenvalues and eigenvectors of a general matrix
//------------------------------------------------------------------------------
// LAPACK's path, as its zgeev takes it (S269): the matrix scaled by a power
// of two so that its largest part lies in [1, 2), balanced (zgebal), reduced
// to upper Hessenberg form by Householder reflectors (zgehd2), its complex
// Schur form found by single-shift QR (zlahqr), the subdiagonal kept real;
// then the eigenvectors from the Schur form by back-substitution, as ztrevc,
// back-transformed (zgebak). The eigenvalues ascend by real part, then by
// imaginary part. The scaling is exact, so 2^k A has 2^k times A's
// eigenvalues, bit for bit, barring overflow and underflow. Each large
// kernel is compiled once in a translation unit (D_MATH_FN_KERNEL), and
// cplx.h's product and quotient through it, so that its many calls are not
// each a copy. What takes a root comes twice, the same bit for bit.
"""

CMAT_GENERAL_ONCE = r"""
// d_internal_cmat_@S@_swap_isolated
//   the swap balancing makes to isolate an eigenvalue: columns _i and _j in
// rows 0 to _l, then rows _i and _j in columns _k to n - 1 - a similarity
// by a permutation.
D_MATH_FN void
d_internal_cmat_@S@_swap_isolated(
    struct dj_cplx_@S@* _a,
    size_t              _n,
    size_t              _i,
    size_t              _j,
    size_t              _k,
    size_t              _l
)
{
    size_t r = 0u;

    for (r = 0u; r <= _l; ++r)
    {
        const struct dj_cplx_@S@ tmp = _a[(r * _n) + _i];

        _a[(r * _n) + _i] = _a[(r * _n) + _j];
        _a[(r * _n) + _j] = tmp;
    }

    for (r = _k; r < _n; ++r)
    {
        const struct dj_cplx_@S@ tmp = _a[(_i * _n) + r];

        _a[(_i * _n) + r] = _a[(_j * _n) + r];
        _a[(_j * _n) + r] = tmp;
    }

    return;
}

// d_cmat_@S@_balance_back
//   undoes d_cmat_@S@_balance on the n-by-_m row-major _v, whose columns
// are right eigenvectors of the balanced matrix, as LAPACK's zgebak does:
// each row from _ilo to _ihi multiplied by its scaling factor, exactly, then
// the permutations undone, the last first.
D_MATH_FN_KERNEL void
d_cmat_@S@_balance_back(
    struct dj_cplx_@S@* _v,
    size_t              _n,
    size_t              _m,
    const size_t*       _perm,
    const @T@*          _scale,
    size_t              _ilo,
    size_t              _ihi
)
{
    size_t i  = 0u;
    size_t c  = 0u;
    size_t ii = 0u;

    // nothing to undo
    if (_n == 0u)
    {
        return;
    }

    // the scaling
    for (i = _ilo; i <= _ihi; ++i)
    {
        for (c = 0u; c < _m; ++c)
        {
            _v[(i * _m) + c] = d_cplx_@S@_mul_real(_v[(i * _m) + c],
                                                   _scale[i]);
        }
    }

    // the permutations, the last first
    for (ii = 0u; ii < _n; ++ii)
    {
        size_t row   = ii;
        size_t other = 0u;

        // inside the balanced block: no swap
        if ( (row >= _ilo) &&
             (row <= _ihi) )
        {
            continue;
        }

        // a column pushed to the top: the last pushed first
        if (row < _ilo)
        {
            row = _ilo - 1u - ii;
        }

        other = _perm[row];

        // swapped with itself: nothing to undo
        if (other == row)
        {
            continue;
        }

        for (c = 0u; c < _m; ++c)
        {
            const struct dj_cplx_@S@ tmp = _v[(row * _m) + c];

            _v[(row * _m) + c]   = _v[(other * _m) + c];
            _v[(other * _m) + c] = tmp;
        }
    }

    return;
}

// d_internal_cmat_@S@_scaling_of
//   decomp.h's d_internal_decomp_@S@_scaling_of for the _count complex
// entries of _a: their parts' largest magnitude's binade, found exactly,
// after a lift by 2^P where that magnitude is subnormal; not finite where a
// part is NaN or infinite, zero where every part is zero.
D_MATH_FN struct dj_internal_decomp_@S@_scaling
d_internal_cmat_@S@_scaling_of(
    const struct dj_cplx_@S@* _a,
    size_t                    _count
)
{
    struct dj_internal_decomp_@S@_scaling result =
        { (@T@)1, (@T@)1, true, false };
    @T@    largest = (@T@)0;
    size_t i       = 0u;
    size_t p       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        for (p = 0u; p < 2u; ++p)
        {
            const @T@ v = d_internal_math_@S@_abs(_a[i].part[p]);

            // a NaN, or an infinity: nothing to scale
            if ( (v != v) ||
                 (v > @MAX@) )
            {
                result.finite = false;

                return result;
            }

            if (v > largest)
            {
                largest = v;
            }
        }
    }

    // every part zero
    if (largest == (@T@)0)
    {
        result.zero = true;

        return result;
    }

    // a subnormal largest: lifted into the normal range first
    if (largest < @MIN@)
    {
        result.pre = d_internal_math_@S@_pow2(@P@);
        largest    = largest * result.pre;
    }

    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(largest);

        result.power = b.power;
    }

    return result;
}

// d_internal_cmat_@S@_turn
//   the _count entries _x[0], _x[_stride], ... each multiplied by _s, as
// BLAS's zscal: the phase LAPACK's zlahqr turns rows and columns by, to keep
// the subdiagonal real.
D_MATH_FN void
d_internal_cmat_@S@_turn(
    struct dj_cplx_@S@* _x,
    size_t              _count,
    size_t              _stride,
    struct dj_cplx_@S@  _s
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _x[i * _stride] = d_internal_cmat_@S@_mul(_s, _x[i * _stride]);
    }

    return;
}
"""

CMAT_GENERAL_FORMS = r"""
// d_internal_cmat_@S@_abs@F@
//   |_z|, the modulus, by d_internal_decomp_@S@_hypot@F@: neither
// overflowing nor underflowing on the way, the same bits in both forms.
D_MATH_FN@RT@ @T@
d_internal_cmat_@S@_abs@F@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_decomp_@S@_hypot@F@(_z.part[0], _z.part[1]);
}

// d_internal_cmat_@S@_sqrt@F@
//   the principal square root of _z, as the shifts need it: t = sqrt((|z| +
// |x|) / 2), each half taken first, then (t, y / (2 t)) where x >= 0, else
// (|y| / (2 t), +-t) with y's sign - on the core's real roots, the same bits
// in both forms; 0 for 0.
D_MATH_FN@RT@ struct dj_cplx_@S@
d_internal_cmat_@S@_sqrt@F@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x = _z.part[0];
    const @T@ y = _z.part[1];

    // zero: zero
    if (d_internal_cmat_@S@_is_zero(_z))
    {
        return d_cplx_@S@_make((@T@)0, (@T@)0);
    }

    {
        const @T@ m    = d_internal_decomp_@S@_hypot@F@(x, y);
        const @T@ ax   = d_internal_math_@S@_abs(x);
        const @T@ hm   = m / (@T@)2;
        const @T@ hx   = ax / (@T@)2;
        const @T@ sum  = hm + hx;
        const @T@ t    = d_math_@S@_sqrt@F@(sum);
        const @T@ twot = (@T@)2 * t;

        if (x >= (@T@)0)
        {
            return d_cplx_@S@_make(t, y / twot);
        }

        {
            const @T@ ay = d_internal_math_@S@_abs(y);

            return d_cplx_@S@_make(ay / twot, (y < (@T@)0) ? -t : t);
        }
    }
}

// d_internal_cmat_@S@_hypot3@F@
//   sqrt(_x^2 + _y^2 + _z^2), as LAPACK's dlapy3: the largest magnitude
// times the root of the sum of the squared ratios to it; the sum of the
// magnitudes where that is zero.
D_MATH_FN@RT@ @T@
d_internal_cmat_@S@_hypot3@F@(
    @T@ _x,
    @T@ _y,
    @T@ _z
)
{
    const @T@ ax = d_internal_math_@S@_abs(_x);
    const @T@ ay = d_internal_math_@S@_abs(_y);
    const @T@ az = d_internal_math_@S@_abs(_z);
    const @T@ w1 = (ax > ay) ? ax : ay;
    const @T@ w  = (w1 > az) ? w1 : az;

    // all zero, or an infinity
    if ( (w == (@T@)0) ||
         (w > @MAX@) )
    {
        const @T@ s = ax + ay;

        return s + az;
    }

    {
        const @T@ rx = ax / w;
        const @T@ ry = ay / w;
        const @T@ rz = az / w;
        const @T@ xx = rx * rx;
        const @T@ yy = ry * ry;
        const @T@ zz = rz * rz;
        const @T@ s1 = xx + yy;
        const @T@ s2 = s1 + zz;
        const @T@ r  = d_math_@S@_sqrt@F@(s2);

        return w * r;
    }
}

// d_cmat_@S@_balance@F@
//   balances the complex n-by-n _a in place, as LAPACK's zgebal does with
// job 'B', and decomp.h's d_decomp_@S@_balance@F@ for a real one: the rows
// and columns that isolate an eigenvalue permuted to the ends, then each
// row and column inside *_ilo..*_ihi scaled by a power of two while that
// brings the sum of their norms below 0.95 of what it was. _scale (n)
// receives the factors, 1 outside; _perm (n), unless NULL, each row's swap.
// Indices count from 0.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_balance@F@(
    struct dj_cplx_@S@* _a,
    size_t              _n,
    size_t*             _perm,
    @T@*                _scale,
    size_t*             _ilo,
    size_t*             _ihi
)
{
    const @T@ sfmin1 = @MIN@ / @EPS@;
    const @T@ sfmax1 = (@T@)1 / sfmin1;
    const @T@ sfmin2 = sfmin1 * (@T@)2;
    const @T@ sfmax2 = (@T@)1 / sfmin2;
    size_t    k      = 0u;
    size_t    l      = 0u;
    size_t    i      = 0u;
    size_t    j      = 0u;
    bool      noconv = true;

    *_ilo = 0u;
    *_ihi = 0u;

    // nothing to balance
    if (_n == 0u)
    {
        return;
    }

    for (i = 0u; i < _n; ++i)
    {
        _scale[i] = (@T@)1;

        if (_perm != NULL)
        {
            _perm[i] = i;
        }
    }

    l = _n - 1u;

    // rows isolating an eigenvalue, pushed to the bottom
    while (noconv)
    {
        const size_t top = l;

        noconv = false;

        for (i = top + 1u; i-- > 0u; )
        {
            bool canswap = true;

            for (j = 0u; j <= l; ++j)
            {
                if ( (i != j) &&
                     (!d_internal_cmat_@S@_is_zero(_a[(i * _n) + j])) )
                {
                    canswap = false;

                    break;
                }
            }

            // the row is zero off the diagonal: to the bottom
            if (canswap)
            {
                if (_perm != NULL)
                {
                    _perm[l] = i;
                }

                if (i != l)
                {
                    d_internal_cmat_@S@_swap_isolated(_a, _n, i, l, k, l);
                }

                noconv = true;

                // every row isolated: the matrix is triangular
                if (l == 0u)
                {
                    return;
                }

                --l;
            }
        }
    }

    // columns isolating an eigenvalue, pushed to the top
    noconv = true;

    while (noconv)
    {
        const size_t first = k;

        noconv = false;

        for (j = first; j <= l; ++j)
        {
            bool canswap = true;

            for (i = k; i <= l; ++i)
            {
                if ( (i != j) &&
                     (!d_internal_cmat_@S@_is_zero(_a[(i * _n) + j])) )
                {
                    canswap = false;

                    break;
                }
            }

            // the column is zero off the diagonal: to the top
            if (canswap)
            {
                if (_perm != NULL)
                {
                    _perm[k] = j;
                }

                if (j != k)
                {
                    d_internal_cmat_@S@_swap_isolated(_a, _n, j, k, k, l);
                }

                noconv = true;
                ++k;
            }
        }
    }

    // every column isolated: one row is left, alone
    if (k > l)
    {
        k = l;
    }

    // the norms brought near, row and column by turns
    noconv = true;

    while (noconv)
    {
        noconv = false;

        for (i = k; i <= l; ++i)
        {
            const size_t count = (l - k) + 1u;
            @T@          c     = d_internal_decomp_@S@_cnorm@F@(
                                     _a + ((k * _n) + i), count, _n);
            @T@          r     = d_internal_decomp_@S@_cnorm@F@(
                                     _a + ((i * _n) + k), count, 1u);
            @T@          ca    = (@T@)0;
            @T@          ra    = (@T@)0;
            @T@          g     = (@T@)0;
            @T@          f     = (@T@)1;
            @T@          s     = (@T@)0;

            // the modulus of the entry of largest |re| + |im|, the first
            {
                size_t best = 0u;

                for (j = 1u; j <= l; ++j)
                {
                    if (d_internal_cmat_@S@_abs1(_a[(j * _n) + i]) >
                        d_internal_cmat_@S@_abs1(_a[(best * _n) + i]))
                    {
                        best = j;
                    }
                }

                ca = d_internal_cmat_@S@_abs@F@(_a[(best * _n) + i]);
                best = k;

                for (j = k + 1u; j < _n; ++j)
                {
                    if (d_internal_cmat_@S@_abs1(_a[(i * _n) + j]) >
                        d_internal_cmat_@S@_abs1(_a[(i * _n) + best]))
                    {
                        best = j;
                    }
                }

                ra = d_internal_cmat_@S@_abs@F@(_a[(i * _n) + best]);
            }

            // a zero norm, through underflow: nothing to bring near
            if ( (c == (@T@)0) ||
                 (r == (@T@)0) )
            {
                continue;
            }

            g = r / (@T@)2;
            s = c + r;

            // the column too small: doubled while it stays below half the row
            while (c < g)
            {
                const @T@ big1  = (f > c) ? f : c;
                const @T@ big   = (big1 > ca) ? big1 : ca;
                const @T@ tiny1 = (r < g) ? r : g;
                const @T@ tiny  = (tiny1 < ra) ? tiny1 : ra;

                if ( (!(big < sfmax2)) ||
                     (!(tiny > sfmin2)) )
                {
                    break;
                }

                f  = f * (@T@)2;
                c  = c * (@T@)2;
                ca = ca * (@T@)2;
                r  = r / (@T@)2;
                g  = g / (@T@)2;
                ra = ra / (@T@)2;
            }

            g = c / (@T@)2;

            // the column too large: halved while half of it is at least the row
            while (g >= r)
            {
                const @T@ big   = (r > ra) ? r : ra;
                const @T@ tiny1 = (f < c) ? f : c;
                const @T@ tiny2 = (g < ca) ? g : ca;
                const @T@ tiny  = (tiny1 < tiny2) ? tiny1 : tiny2;

                if ( (!(big < sfmax2)) ||
                     (!(tiny > sfmin2)) )
                {
                    break;
                }

                f  = f / (@T@)2;
                c  = c / (@T@)2;
                g  = g / (@T@)2;
                ca = ca / (@T@)2;
                r  = r * (@T@)2;
                ra = ra * (@T@)2;
            }

            {
                const @T@ sum   = c + r;
                const @T@ bound = (@T@)0.95 * s;

                // not enough gained
                if (sum >= bound)
                {
                    continue;
                }
            }

            // the factor would leave the safe range
            if ( (f < (@T@)1) &&
                 (_scale[i] < (@T@)1) )
            {
                const @T@ product = f * _scale[i];

                if (product <= sfmin1)
                {
                    continue;
                }
            }

            if ( (f > (@T@)1) &&
                 (_scale[i] > (@T@)1) )
            {
                const @T@ room = sfmax1 / f;

                if (_scale[i] >= room)
                {
                    continue;
                }
            }

            g         = (@T@)1 / f;
            _scale[i] = _scale[i] * f;
            noconv    = true;

            for (j = k; j < _n; ++j)
            {
                _a[(i * _n) + j] = d_cplx_@S@_mul_real(_a[(i * _n) + j], g);
            }

            for (j = 0u; j <= l; ++j)
            {
                _a[(j * _n) + i] = d_cplx_@S@_mul_real(_a[(j * _n) + i], f);
            }
        }
    }

    *_ilo = k;
    *_ihi = l;

    return;
}

// d_internal_cmat_@S@_reflector@F@
//   the elementary reflector H = I - tau v v^H, v = (1, x), x the _count
// entries _x[0], _x[_stride], ..., that takes (*_alpha, x) to (beta, 0) with
// beta real, as LAPACK's zlarfg: beta = -sign(re alpha) times the norm of
// (alpha, x), tau = ((beta - re alpha) / beta, -im alpha / beta), x
// multiplied by 1 / (alpha - beta); lifted first where beta is below the
// family's least normal over its epsilon, and lowered after. *_alpha
// becomes beta, _x v's tail, and tau is returned: 0, H = I, where x is zero
// and alpha real.
D_MATH_FN_KERNEL@RT@ struct dj_cplx_@S@
d_internal_cmat_@S@_reflector@F@(
    struct dj_cplx_@S@* _alpha,
    struct dj_cplx_@S@* _x,
    size_t              _count,
    size_t              _stride
)
{
    const @T@ safmin = @MIN@ / (@EPS@ / (@T@)2);
    const @T@ rsafmn = (@T@)1 / safmin;
    @T@       xnorm  = d_internal_decomp_@S@_cnorm@F@(_x, _count, _stride);
    @T@       alphr  = _alpha->part[0];
    @T@       alphi  = _alpha->part[1];
    @T@       beta   = (@T@)0;
    size_t    knt    = 0u;
    size_t    i      = 0u;
    struct dj_cplx_@S@ tau = d_cplx_@S@_make((@T@)0, (@T@)0);

    // x zero and alpha real: H = I
    if ( (xnorm == (@T@)0) &&
         (alphi == (@T@)0) )
    {
        return tau;
    }

    {
        const @T@ h = d_internal_cmat_@S@_hypot3@F@(alphr, alphi, xnorm);

        beta = (alphr < (@T@)0) ? h : -h;
    }

    // beta below the safe minimum: lifted, and found again
    if (d_internal_math_@S@_abs(beta) < safmin)
    {
        do
        {
            ++knt;

            for (i = 0u; i < _count; ++i)
            {
                _x[i * _stride] = d_cplx_@S@_mul_real(_x[i * _stride],
                                                      rsafmn);
            }

            beta  = beta * rsafmn;
            alphi = alphi * rsafmn;
            alphr = alphr * rsafmn;
        }
        while ( (d_internal_math_@S@_abs(beta) < safmin) &&
                (knt < 20u) );

        xnorm = d_internal_decomp_@S@_cnorm@F@(_x, _count, _stride);

        {
            const @T@ h = d_internal_cmat_@S@_hypot3@F@(alphr, alphi, xnorm);

            beta = (alphr < (@T@)0) ? h : -h;
        }
    }

    {
        const @T@ diff = beta - alphr;
        const @T@ mi   = -alphi;
        const struct dj_cplx_@S@ rec = d_internal_cmat_@S@_div(
            d_cplx_@S@_make((@T@)1, (@T@)0),
            d_cplx_@S@_make(alphr - beta, alphi));

        tau = d_cplx_@S@_make(diff / beta, mi / beta);

        for (i = 0u; i < _count; ++i)
        {
            _x[i * _stride] = d_internal_cmat_@S@_mul(rec, _x[i * _stride]);
        }
    }

    // lowered back
    for (i = 0u; i < knt; ++i)
    {
        beta = beta * safmin;
    }

    *_alpha = d_cplx_@S@_make(beta, (@T@)0);

    return tau;
}

// d_cmat_@S@_hessenberg_q@F@
//   reduces rows and columns _ilo to _ihi of the complex n-by-n _h to upper
// Hessenberg form in place by Householder reflectors, as LAPACK's zgehd2:
// for each column from _ilo, the reflector H that zeroes it below the
// subdiagonal and makes the subdiagonal entry real, H applied from the right
// to rows 0 to _ihi and H^H from the left to columns to the end - a
// similarity, H = Q^H A Q. _q, unless NULL, the n-by-n identity on entry,
// receives Q, each reflector multiplied in on the right as it is made. The
// entries below the subdiagonal end exactly zero.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_hessenberg_q@F@(
    struct dj_cplx_@S@* _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    struct dj_cplx_@S@* _q
)
{
    size_t i = 0u;

    // nothing to reduce
    if ( (_n == 0u) ||
         (_ihi <= _ilo) )
    {
        return;
    }

    for (i = _ilo; i < _ihi; ++i)
    {
        const size_t             below = _ihi - i - 1u;
        struct dj_cplx_@S@       alpha = _h[((i + 1u) * _n) + i];
        const struct dj_cplx_@S@ tau   = d_internal_cmat_@S@_reflector@F@(
            &alpha,
            _h + (((((i + 2u) < _n) ? (i + 2u) : (_n - 1u)) * _n) + i),
            below,
            _n);
        const struct dj_cplx_@S@ ctau  = d_cplx_@S@_conj(tau);
        size_t                   r     = 0u;
        size_t                   c     = 0u;

        _h[((i + 1u) * _n) + i] = alpha;

        // H = I: nothing to apply
        if (!d_internal_cmat_@S@_is_zero(tau))
        {
            // from the right, H: rows 0 to ihi, columns i + 1 to ihi
            for (r = 0u; r <= _ihi; ++r)
            {
                struct dj_cplx_@S@ w = _h[(r * _n) + (i + 1u)];

                for (c = i + 2u; c <= _ihi; ++c)
                {
                    w = d_cplx_@S@_add(w, d_internal_cmat_@S@_mul(_h[(r * _n) + c],
                                                         _h[(c * _n) + i]));
                }

                {
                    const struct dj_cplx_@S@ t =
                        d_internal_cmat_@S@_mul(tau, w);

                    _h[(r * _n) + (i + 1u)] =
                        d_cplx_@S@_sub(_h[(r * _n) + (i + 1u)], t);

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        _h[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                            _h[(r * _n) + c], t,
                            d_cplx_@S@_conj(_h[(c * _n) + i]));
                    }
                }
            }

            // from the left, H^H: rows i + 1 to ihi, columns i + 1 to the end
            for (c = i + 1u; c < _n; ++c)
            {
                struct dj_cplx_@S@ w = _h[((i + 1u) * _n) + c];

                for (r = i + 2u; r <= _ihi; ++r)
                {
                    w = d_cplx_@S@_add(w, d_internal_cmat_@S@_mul(
                                              d_cplx_@S@_conj(
                                                  _h[(r * _n) + i]),
                                              _h[(r * _n) + c]));
                }

                {
                    const struct dj_cplx_@S@ t = d_internal_cmat_@S@_mul(ctau, w);

                    _h[((i + 1u) * _n) + c] =
                        d_cplx_@S@_sub(_h[((i + 1u) * _n) + c], t);

                    for (r = i + 2u; r <= _ihi; ++r)
                    {
                        _h[(r * _n) + c] = d_internal_cmat_@S@_less_product(
                            _h[(r * _n) + c], _h[(r * _n) + i], t);
                    }
                }
            }

            // Q's columns i + 1 to ihi, from the right
            if (_q != NULL)
            {
                for (r = _ilo; r <= _ihi; ++r)
                {
                    struct dj_cplx_@S@ w = _q[(r * _n) + (i + 1u)];

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        w = d_cplx_@S@_add(w, d_internal_cmat_@S@_mul(
                                                  _q[(r * _n) + c],
                                                  _h[(c * _n) + i]));
                    }

                    {
                        const struct dj_cplx_@S@ t =
                            d_internal_cmat_@S@_mul(tau, w);

                        _q[(r * _n) + (i + 1u)] =
                            d_cplx_@S@_sub(_q[(r * _n) + (i + 1u)], t);

                        for (c = i + 2u; c <= _ihi; ++c)
                        {
                            _q[(r * _n) + c] =
                                d_internal_cmat_@S@_less_product(
                                    _q[(r * _n) + c], t,
                                    d_cplx_@S@_conj(_h[(c * _n) + i]));
                        }
                    }
                }
            }
        }

        // the reflector's vector, kept below the subdiagonal until now
        for (r = i + 2u; r <= _ihi; ++r)
        {
            _h[(r * _n) + i] = d_cplx_@S@_make((@T@)0, (@T@)0);
        }
    }

    return;
}

// d_cmat_@S@_schur@F@
//   the eigenvalues of the complex n-by-n upper Hessenberg _h, rows and
// columns _ilo to _ihi its unreduced part (the rest triangular), into
// _values, by single-shift complex QR, as LAPACK's zlahqr: the subdiagonal
// made real first, by turning rows and columns; then from the bottom up, a
// subdiagonal entry negligible by Ahues and Kressner's test - relative to
// its neighbours, _tol below the family's epsilon taken as the epsilon -
// set to zero and the eigenvalue below it split off, or else a single-shift
// step on the unreduced block from the row where two small subdiagonal
// entries let it start: Wilkinson's shift, the trailing 2-by-2's eigenvalue
// nearer its last entry, and at every tenth step without a split an
// exceptional shift; the subdiagonal kept real. At most _max_iter steps in
// all. With _z NULL only the eigenvalues are found; else _h ends upper
// triangular, its complex Schur form, and the n-by-n _z, Q on entry, is
// multiplied by each transformation. _values receives the eigenvalues in the
// order of T's diagonal. Returns whether all were found; if not, the rows
// not reached take their diagonal entries.
D_MATH_FN_KERNEL@RT@ bool
d_cmat_@S@_schur@F@(
    struct dj_cplx_@S@* _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    struct dj_cplx_@S@* _z,
    struct dj_cplx_@S@* _values,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const @T@    dat1   = (@T@)0.75;
    const size_t kexsh  = 10u;
    const @T@    ulp    = @EPS@;
    const @T@    tol    = (_tol > ulp) ? _tol : ulp;
    const bool   wantt  = (_z != NULL);
    const struct dj_cplx_@S@ zero = d_cplx_@S@_make((@T@)0, (@T@)0);
    size_t       nh     = 0u;
    @T@          smlnum = (@T@)0;
    size_t       i1     = 0u;
    size_t       i2     = 0u;
    size_t       kdefl  = 0u;
    size_t       steps  = 0u;
    size_t       top    = 0u;
    size_t       j      = 0u;

    // nothing to find
    if (_n == 0u)
    {
        return true;
    }

    // the eigenvalues outside the unreduced part: the diagonal
    for (j = 0u; j < _n; ++j)
    {
        if ( (j < _ilo) ||
             (j > _ihi) )
        {
            _values[j] = _h[(j * _n) + j];
        }
    }

    // a block of one
    if (_ilo == _ihi)
    {
        _values[_ilo] = _h[(_ilo * _n) + _ilo];

        return true;
    }

    // what lies below the subdiagonal, cleared
    for (j = _ilo; (j + 3u) <= _ihi; ++j)
    {
        _h[((j + 2u) * _n) + j] = zero;
        _h[((j + 3u) * _n) + j] = zero;
    }

    if ((_ilo + 2u) <= _ihi)
    {
        _h[(_ihi * _n) + (_ihi - 2u)] = zero;
    }

    // the subdiagonal made real, each entry turned by its phase
    {
        const size_t jlo = wantt ? 0u : _ilo;
        const size_t jhi = wantt ? (_n - 1u) : _ihi;
        size_t       i   = 0u;

        for (i = _ilo + 1u; i <= _ihi; ++i)
        {
            const struct dj_cplx_@S@ h = _h[(i * _n) + (i - 1u)];

            if (h.part[1] != (@T@)0)
            {
                const struct dj_cplx_@S@ s1 =
                    d_cplx_@S@_div_real(h, d_internal_cmat_@S@_abs1(h));
                const struct dj_cplx_@S@ sc =
                    d_cplx_@S@_div_real(d_cplx_@S@_conj(s1),
                                        d_internal_cmat_@S@_abs@F@(s1));
                const size_t last = (jhi < (i + 1u)) ? jhi : (i + 1u);

                _h[(i * _n) + (i - 1u)] =
                    d_cplx_@S@_make(d_internal_cmat_@S@_abs@F@(h), (@T@)0);
                d_internal_cmat_@S@_turn(_h + ((i * _n) + i), (jhi - i) + 1u,
                                         1u, sc);
                d_internal_cmat_@S@_turn(_h + ((jlo * _n) + i),
                                         (last - jlo) + 1u, _n,
                                         d_cplx_@S@_conj(sc));

                if (_z != NULL)
                {
                    d_internal_cmat_@S@_turn(_z + i, _n, _n,
                                             d_cplx_@S@_conj(sc));
                }
            }
        }
    }

    nh     = (_ihi - _ilo) + 1u;
    smlnum = @MIN@ * ((@T@)nh / ulp);

    if (wantt)
    {
        i1 = 0u;
        i2 = _n - 1u;
    }

    // top is one past the last row still to be solved
    top = _ihi + 1u;

    while (top > _ilo)
    {
        const size_t i = top - 1u;
        size_t       l = _ilo;
        size_t       k = 0u;

        for (;;)
        {
            size_t             m = 0u;
            struct dj_cplx_@S@ v[2] = { { { (@T@)0, (@T@)0 } },
                                        { { (@T@)0, (@T@)0 } } };
            struct dj_cplx_@S@ t    = zero;

            // the lowest negligible subdiagonal entry
            for (k = i; k > l; --k)
            {
                const struct dj_cplx_@S@ hk = _h[(k * _n) + (k - 1u)];
                @T@                      tst = (@T@)0;

                if (d_internal_cmat_@S@_abs1(hk) <= smlnum)
                {
                    break;
                }

                {
                    const @T@ d1 = d_internal_cmat_@S@_abs1(
                                       _h[((k - 1u) * _n) + (k - 1u)]);
                    const @T@ d2 = d_internal_cmat_@S@_abs1(
                                       _h[(k * _n) + k]);

                    tst = d1 + d2;
                }

                // both neighbours zero: the next ones out
                if (tst == (@T@)0)
                {
                    if (k >= (_ilo + 2u))
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k - 1u) * _n) + (k - 2u)].part[0]);
                    }

                    if ((k + 1u) <= _ihi)
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k + 1u) * _n) + k].part[0]);
                    }
                }

                {
                    const @T@ sub   = d_internal_math_@S@_abs(hk.part[0]);
                    const @T@ bound = tol * tst;

                    // Ahues and Kressner's test
                    if (sub <= bound)
                    {
                        const @T@ e1   = d_internal_cmat_@S@_abs1(hk);
                        const @T@ e2   = d_internal_cmat_@S@_abs1(
                                             _h[((k - 1u) * _n) + k]);
                        const @T@ ab   = (e1 > e2) ? e1 : e2;
                        const @T@ ba   = (e1 > e2) ? e2 : e1;
                        const @T@ hkk  = d_internal_cmat_@S@_abs1(
                                             _h[(k * _n) + k]);
                        const @T@ ad   = d_internal_cmat_@S@_abs1(
                                             d_cplx_@S@_sub(
                                                 _h[((k - 1u) * _n) +
                                                    (k - 1u)],
                                                 _h[(k * _n) + k]));
                        const @T@ aa   = (hkk > ad) ? hkk : ad;
                        const @T@ bb   = (hkk > ad) ? ad : hkk;
                        const @T@ s    = aa + ab;
                        const @T@ r1   = ab / s;
                        const @T@ lhs  = ba * r1;
                        const @T@ r2   = aa / s;
                        const @T@ p    = bb * r2;
                        const @T@ rhs1 = tol * p;
                        const @T@ rhs  = (smlnum > rhs1) ? smlnum : rhs1;

                        if (lhs <= rhs)
                        {
                            break;
                        }
                    }
                }
            }

            l = k;

            // negligible: zero
            if (l > _ilo)
            {
                _h[(l * _n) + (l - 1u)] = zero;
            }

            // an eigenvalue split off
            if (l >= i)
            {
                break;
            }

            // the budget spent: the rows not reached, their diagonal
            if (steps >= _max_iter)
            {
                for (j = _ilo; j <= i; ++j)
                {
                    _values[j] = _h[(j * _n) + j];
                }

                return false;
            }

            ++steps;
            ++kdefl;

            // eigenvalues alone: only the active block transformed
            if (!wantt)
            {
                i1 = l;
                i2 = i;
            }

            // exceptional shifts, at the bottom, then at the top
            if ((kdefl % (2u * kexsh)) == 0u)
            {
                const @T@ s = dat1 * d_internal_math_@S@_abs(
                                         _h[(i * _n) + (i - 1u)].part[0]);

                t = d_cplx_@S@_add_real(_h[(i * _n) + i], s);
            }
            else if ((kdefl % kexsh) == 0u)
            {
                const @T@ s = dat1 * d_internal_math_@S@_abs(
                                         _h[((l + 1u) * _n) + l].part[0]);

                t = d_cplx_@S@_add_real(_h[(l * _n) + l], s);
            }
            // Wilkinson's
            else
            {
                const struct dj_cplx_@S@ u = d_internal_cmat_@S@_mul(
                    d_internal_cmat_@S@_sqrt@F@(_h[((i - 1u) * _n) + i]),
                    d_internal_cmat_@S@_sqrt@F@(_h[(i * _n) + (i - 1u)]));
                @T@ s = d_internal_cmat_@S@_abs1(u);

                t = _h[(i * _n) + i];

                if (s != (@T@)0)
                {
                    const struct dj_cplx_@S@ x = d_cplx_@S@_mul_real(
                        d_cplx_@S@_sub(_h[((i - 1u) * _n) + (i - 1u)], t),
                        (@T@)0.5);
                    const @T@ sx = d_internal_cmat_@S@_abs1(x);
                    struct dj_cplx_@S@ y = zero;

                    s = (s > sx) ? s : sx;

                    {
                        const struct dj_cplx_@S@ xs = d_cplx_@S@_div_real(x, s);
                        const struct dj_cplx_@S@ us = d_cplx_@S@_div_real(u, s);
                        const struct dj_cplx_@S@ sq = d_cplx_@S@_add(
                            d_internal_cmat_@S@_mul(xs, xs), d_internal_cmat_@S@_mul(us, us));

                        y = d_cplx_@S@_mul_real(
                            d_internal_cmat_@S@_sqrt@F@(sq), s);
                    }

                    // y's sign: the root nearer x
                    if (sx > (@T@)0)
                    {
                        const struct dj_cplx_@S@ xn = d_cplx_@S@_div_real(x, sx);
                        const @T@ p1  = xn.part[0] * y.part[0];
                        const @T@ p2  = xn.part[1] * y.part[1];
                        const @T@ dot = p1 + p2;

                        if (dot < (@T@)0)
                        {
                            y = d_cplx_@S@_neg(y);
                        }
                    }

                    t = d_internal_cmat_@S@_less_product(
                        t, u, d_internal_cmat_@S@_div(u, d_cplx_@S@_add(x, y)));
                }
            }

            // the row the step starts at: two small subdiagonal entries
            for (m = i - 1u; m > l; --m)
            {
                const struct dj_cplx_@S@ h11  = _h[(m * _n) + m];
                const struct dj_cplx_@S@ h22  = _h[((m + 1u) * _n) + (m + 1u)];
                struct dj_cplx_@S@       h11s = d_cplx_@S@_sub(h11, t);
                @T@                      h21  = _h[((m + 1u) * _n) + m].part[0];
                const @T@                h10  = _h[(m * _n) + (m - 1u)].part[0];
                @T@                      s    = (@T@)0;

                {
                    const @T@ a1 = d_internal_cmat_@S@_abs1(h11s);
                    const @T@ a2 = d_internal_math_@S@_abs(h21);

                    s = a1 + a2;
                }

                h11s = d_cplx_@S@_div_real(h11s, s);
                h21  = h21 / s;
                v[0] = h11s;
                v[1] = d_cplx_@S@_make(h21, (@T@)0);

                {
                    const @T@ a10 = d_internal_math_@S@_abs(h10);
                    const @T@ a21 = d_internal_math_@S@_abs(h21);
                    const @T@ lhs = a10 * a21;
                    const @T@ e1  = d_internal_cmat_@S@_abs1(h11s);
                    const @T@ e2  = d_internal_cmat_@S@_abs1(h11);
                    const @T@ e3  = d_internal_cmat_@S@_abs1(h22);
                    const @T@ e23 = e2 + e3;
                    const @T@ p   = e1 * e23;
                    const @T@ rhs = ulp * p;

                    // starting here leaves H(m, m - 1) negligible
                    if (lhs <= rhs)
                    {
                        break;
                    }
                }
            }

            // the top of the block
            if (m == l)
            {
                const struct dj_cplx_@S@ h11  = _h[(l * _n) + l];
                struct dj_cplx_@S@       h11s = d_cplx_@S@_sub(h11, t);
                @T@                      h21  = _h[((l + 1u) * _n) + l].part[0];
                @T@                      s    = (@T@)0;

                {
                    const @T@ a1 = d_internal_cmat_@S@_abs1(h11s);
                    const @T@ a2 = d_internal_math_@S@_abs(h21);

                    s = a1 + a2;
                }

                h11s = d_cplx_@S@_div_real(h11s, s);
                h21  = h21 / s;
                v[0] = h11s;
                v[1] = d_cplx_@S@_make(h21, (@T@)0);
            }

            // the single-shift step: the bulge made, then chased out
            for (k = m; k < i; ++k)
            {
                struct dj_cplx_@S@ t1   = zero;
                struct dj_cplx_@S@ v2   = zero;
                @T@                t2   = (@T@)0;
                size_t             jj   = 0u;
                size_t             last = ((k + 2u) < i) ? (k + 2u) : i;

                // past the first: the bulge's column, to be reflected
                if (k > m)
                {
                    v[0] = _h[(k * _n) + (k - 1u)];
                    v[1] = _h[((k + 1u) * _n) + (k - 1u)];
                }

                t1 = d_internal_cmat_@S@_reflector@F@(&v[0], &v[1], 1u, 1u);

                if (k > m)
                {
                    _h[(k * _n) + (k - 1u)]        = v[0];
                    _h[((k + 1u) * _n) + (k - 1u)] = zero;
                }

                v2 = v[1];
                t2 = d_internal_cmat_@S@_mul(t1, v2).part[0];

                // from the left: rows k and k + 1, columns k to i2
                for (jj = k; jj <= i2; ++jj)
                {
                    const struct dj_cplx_@S@ sum = d_cplx_@S@_add(
                        d_internal_cmat_@S@_mul(d_cplx_@S@_conj(t1),
                                       _h[(k * _n) + jj]),
                        d_cplx_@S@_mul_real(_h[((k + 1u) * _n) + jj], t2));

                    _h[(k * _n) + jj] = d_cplx_@S@_sub(_h[(k * _n) + jj], sum);
                    _h[((k + 1u) * _n) + jj] = d_internal_cmat_@S@_less_product(
                        _h[((k + 1u) * _n) + jj], sum, v2);
                }

                // from the right: rows i1 to min(k + 2, i)
                for (jj = i1; jj <= last; ++jj)
                {
                    const struct dj_cplx_@S@ sum = d_cplx_@S@_add(
                        d_internal_cmat_@S@_mul(t1, _h[(jj * _n) + k]),
                        d_cplx_@S@_mul_real(_h[(jj * _n) + (k + 1u)], t2));

                    _h[(jj * _n) + k] = d_cplx_@S@_sub(_h[(jj * _n) + k], sum);
                    _h[(jj * _n) + (k + 1u)] = d_internal_cmat_@S@_less_product(
                        _h[(jj * _n) + (k + 1u)], sum, d_cplx_@S@_conj(v2));
                }

                // Z's columns
                if (_z != NULL)
                {
                    for (jj = 0u; jj < _n; ++jj)
                    {
                        const struct dj_cplx_@S@ sum = d_cplx_@S@_add(
                            d_internal_cmat_@S@_mul(t1, _z[(jj * _n) + k]),
                            d_cplx_@S@_mul_real(_z[(jj * _n) + (k + 1u)], t2));

                        _z[(jj * _n) + k] = d_cplx_@S@_sub(_z[(jj * _n) + k],
                                                           sum);
                        _z[(jj * _n) + (k + 1u)] =
                            d_internal_cmat_@S@_less_product(
                                _z[(jj * _n) + (k + 1u)], sum,
                                d_cplx_@S@_conj(v2));
                    }
                }

                // a step begun below the top: H(m, m - 1) kept real
                if ( (k == m) &&
                     (m > l) )
                {
                    const struct dj_cplx_@S@ one_less = d_cplx_@S@_sub(
                        d_cplx_@S@_make((@T@)1, (@T@)0), t1);
                    const struct dj_cplx_@S@ temp = d_cplx_@S@_div_real(
                        one_less, d_internal_cmat_@S@_abs@F@(one_less));
                    const struct dj_cplx_@S@ ctemp = d_cplx_@S@_conj(temp);

                    _h[((m + 1u) * _n) + m] =
                        d_internal_cmat_@S@_mul(_h[((m + 1u) * _n) + m], ctemp);

                    if ((m + 2u) <= i)
                    {
                        _h[((m + 2u) * _n) + (m + 1u)] =
                            d_internal_cmat_@S@_mul(_h[((m + 2u) * _n) + (m + 1u)],
                                           temp);
                    }

                    for (jj = m; jj <= i; ++jj)
                    {
                        if (jj != (m + 1u))
                        {
                            if (i2 > jj)
                            {
                                d_internal_cmat_@S@_turn(
                                    _h + ((jj * _n) + (jj + 1u)), i2 - jj,
                                    1u, temp);
                            }

                            d_internal_cmat_@S@_turn(_h + ((i1 * _n) + jj),
                                                     jj - i1, _n, ctemp);

                            if (_z != NULL)
                            {
                                d_internal_cmat_@S@_turn(_z + jj, _n, _n,
                                                         ctemp);
                            }
                        }
                    }
                }
            }

            // H(i, i - 1) kept real
            {
                const struct dj_cplx_@S@ temp0 = _h[(i * _n) + (i - 1u)];

                if (temp0.part[1] != (@T@)0)
                {
                    const @T@ rtemp = d_internal_cmat_@S@_abs@F@(temp0);
                    const struct dj_cplx_@S@ temp =
                        d_cplx_@S@_div_real(temp0, rtemp);

                    _h[(i * _n) + (i - 1u)] = d_cplx_@S@_make(rtemp, (@T@)0);

                    if (i2 > i)
                    {
                        d_internal_cmat_@S@_turn(_h + ((i * _n) + (i + 1u)),
                                                 i2 - i, 1u,
                                                 d_cplx_@S@_conj(temp));
                    }

                    d_internal_cmat_@S@_turn(_h + ((i1 * _n) + i), i - i1,
                                             _n, temp);

                    if (_z != NULL)
                    {
                        d_internal_cmat_@S@_turn(_z + i, _n, _n, temp);
                    }
                }
            }
        }

        // one eigenvalue
        _values[i] = _h[(i * _n) + i];
        kdefl      = 0u;
        top        = l;
    }

    return true;
}

// d_cmat_@S@_schur_vectors@F@
//   the right eigenvectors of the n-by-n upper triangular complex Schur
// form _t, back-transformed by the Schur vectors, as LAPACK's ztrevc with
// job 'B': _vr holds Z on entry, and on exit the eigenvectors of Z T Z^H,
// column for column, each scaled to a largest |re| + |im| of 1. From the
// last up, each by back-substitution, (T - t_kk I) x = 0 with x_k = 1: a
// divisor smaller than ulp |t_kk| taken as that, and x scaled down wherever
// the next step could overflow, as LAPACK's dtrevc does for its blocks of
// one; then Z x. _x (n entries) and _norms (n) are scratch.
D_MATH_FN_KERNEL@RT@ void
d_cmat_@S@_schur_vectors@F@(
    const struct dj_cplx_@S@* _t,
    size_t                    _n,
    struct dj_cplx_@S@*       _vr,
    struct dj_cplx_@S@*       _x,
    @T@*                      _norms
)
{
    const @T@ ulp    = @EPS@;
    const @T@ smlnum = @MIN@ * ((@T@)_n / ulp);
    const @T@ bignum = ((@T@)1 - ulp) / smlnum;
    size_t    j      = 0u;
    size_t    k      = 0u;
    size_t    r      = 0u;
    size_t    top    = 0u;

    // nothing to find
    if (_n == 0u)
    {
        return;
    }

    // each column's |re| + |im| sum above the diagonal, to foresee overflow
    _norms[0] = (@T@)0;

    for (j = 1u; j < _n; ++j)
    {
        _norms[j] = (@T@)0;

        for (k = 0u; k < j; ++k)
        {
            _norms[j] = _norms[j] +
                        d_internal_cmat_@S@_abs1(_t[(k * _n) + j]);
        }
    }

    for (top = _n; top > 0u; --top)
    {
        const size_t             ki   = top - 1u;
        const struct dj_cplx_@S@ w    = _t[(ki * _n) + ki];
        @T@                      smin = (@T@)0;
        size_t                   jj   = 0u;

        {
            const @T@ u = ulp * d_internal_cmat_@S@_abs1(w);

            smin = (u > smlnum) ? u : smlnum;
        }

        _x[ki] = d_cplx_@S@_make((@T@)1, (@T@)0);

        for (k = 0u; k < ki; ++k)
        {
            _x[k] = d_cplx_@S@_neg(_t[(k * _n) + ki]);
        }

        // (T(0..ki-1, 0..ki-1) - w) x = scale x, the last row first
        for (jj = ki; jj > 0u; --jj)
        {
            const size_t       jr    = jj - 1u;
            struct dj_cplx_@S@ c     = d_cplx_@S@_sub(_t[(jr * _n) + jr], w);
            struct dj_cplx_@S@ xj    = _x[jr];
            @T@                scale = (@T@)1;
            @T@                cnorm = d_internal_cmat_@S@_abs1(c);
            const @T@          bnorm = d_internal_cmat_@S@_abs1(xj);
            @T@                xnorm = (@T@)0;

            // a divisor below smin: smin
            if (cnorm < smin)
            {
                c     = d_cplx_@S@_make(smin, (@T@)0);
                cnorm = smin;
            }

            // b / c would overflow: scaled
            if ( (cnorm < (@T@)1) &&
                 (bnorm > (@T@)1) )
            {
                const @T@ bound = bignum * cnorm;

                if (bnorm > bound)
                {
                    scale = (@T@)1 / bnorm;
                }
            }

            xj    = d_internal_cmat_@S@_div(d_cplx_@S@_mul_real(xj, scale), c);
            xnorm = d_internal_cmat_@S@_abs1(xj);

            // x scaled down where the update could overflow
            if (xnorm > (@T@)1)
            {
                const @T@ bound = bignum / xnorm;

                if (_norms[jr] > bound)
                {
                    xj    = d_cplx_@S@_div_real(xj, xnorm);
                    scale = scale / xnorm;
                }
            }

            if (scale != (@T@)1)
            {
                for (k = 0u; k <= ki; ++k)
                {
                    _x[k] = d_cplx_@S@_mul_real(_x[k], scale);
                }
            }

            _x[jr] = xj;

            for (k = 0u; k < jr; ++k)
            {
                _x[k] = d_internal_cmat_@S@_less_product(
                    _x[k], xj, _t[(k * _n) + jr]);
            }
        }

        // Z x, into the column
        if (ki > 0u)
        {
            const struct dj_cplx_@S@ beta = _x[ki];
            const bool beta_zero = d_internal_cmat_@S@_is_zero(beta);
            const bool beta_one  = ( (beta.part[0] == (@T@)1) &&
                                     (beta.part[1] == (@T@)0) );

            for (r = 0u; r < _n; ++r)
            {
                if (beta_zero)
                {
                    _vr[(r * _n) + ki] = d_cplx_@S@_make((@T@)0, (@T@)0);
                }
                else if (!beta_one)
                {
                    _vr[(r * _n) + ki] = d_internal_cmat_@S@_mul(beta,
                                                        _vr[(r * _n) + ki]);
                }
            }

            for (k = 0u; k < ki; ++k)
            {
                const struct dj_cplx_@S@ temp = _x[k];

                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + ki] = d_cplx_@S@_add(
                        _vr[(r * _n) + ki],
                        d_internal_cmat_@S@_mul(temp, _vr[(r * _n) + k]));
                }
            }
        }

        // the largest |re| + |im| 1
        {
            size_t best  = 0u;
            @T@    remax = (@T@)0;

            for (r = 1u; r < _n; ++r)
            {
                if (d_internal_cmat_@S@_abs1(_vr[(r * _n) + ki]) >
                    d_internal_cmat_@S@_abs1(_vr[(best * _n) + ki]))
                {
                    best = r;
                }
            }

            remax = (@T@)1 / d_internal_cmat_@S@_abs1(_vr[(best * _n) + ki]);

            for (r = 0u; r < _n; ++r)
            {
                _vr[(r * _n) + ki] = d_cplx_@S@_mul_real(_vr[(r * _n) + ki],
                                                         remax);
            }
        }
    }

    return;
}

// d_internal_cmat_@S@_unit_vectors@F@
//   the eigenvectors in the columns of the n-by-n _v made of unit length,
// in place, each turned so that its entry of largest modulus (the first
// among equals) is real and positive.
D_MATH_FN_KERNEL@RT@ void
d_internal_cmat_@S@_unit_vectors@F@(
    struct dj_cplx_@S@* _v,
    size_t              _n
)
{
    size_t j = 0u;
    size_t r = 0u;

    for (j = 0u; j < _n; ++j)
    {
        size_t best    = 0u;
        @T@    largest = (@T@)0;
        @T@    modulus = (@T@)0;
        @T@    norm    = (@T@)0;

        for (r = 0u; r < _n; ++r)
        {
            const @T@ a  = _v[(r * _n) + j].part[0];
            const @T@ b  = _v[(r * _n) + j].part[1];
            const @T@ aa = a * a;
            const @T@ bb = b * b;
            const @T@ m  = aa + bb;

            if ( (r == 0u) ||
                 (m > largest) )
            {
                largest = m;
                best    = r;
            }
        }

        modulus = d_internal_cmat_@S@_abs@F@(_v[(best * _n) + j]);

        // turned so that the largest entry is real and positive
        {
            const @T@ ur = _v[(best * _n) + j].part[0] / modulus;
            const @T@ ui = _v[(best * _n) + j].part[1] / modulus;
            const struct dj_cplx_@S@ u = d_cplx_@S@_make(ur, -ui);

            for (r = 0u; r < _n; ++r)
            {
                _v[(r * _n) + j] = (r == best)
                    ? d_cplx_@S@_make(modulus, (@T@)0)
                    : d_internal_cmat_@S@_mul(_v[(r * _n) + j], u);
            }
        }

        norm = d_internal_decomp_@S@_cnorm@F@(_v + j, _n, _n);

        for (r = 0u; r < _n; ++r)
        {
            _v[(r * _n) + j] = d_cplx_@S@_div_real(_v[(r * _n) + j], norm);
        }

        _v[(best * _n) + j].part[1] = (@T@)0;
    }

    return;
}

// d_cmat_@S@_eigen@F@
//   the eigenvalues of the complex n-by-n _a and, unless _vectors is NULL,
// an eigenvector for each, as LAPACK's zgeev finds them, and decomp.h's
// d_decomp_@S@_eigen@F@ a real matrix's: _a scaled by a power of two to a
// largest part in [1, 2), exactly; balanced (d_cmat_@S@_balance@F@);
// reduced to Hessenberg form (d_cmat_@S@_hessenberg_q@F@); its complex
// Schur form found by single-shift QR (d_cmat_@S@_schur@F@), at most
// _max_iter steps, _tol its deflation's tolerance (the epsilon where
// smaller); and each eigenvector from the Schur form by back-substitution
// (d_cmat_@S@_schur_vectors@F@), back-transformed, of unit length with its
// entry of largest modulus real and positive (the first among equals).
// _values receives the eigenvalues, ascending by real part, then by
// imaginary part, and column j of the row-major n-by-n _vectors the
// eigenvector of _values[j]. _a is consumed; _work is complex scratch of n
// (n + 1) entries, unused where _vectors is NULL; _rwork real scratch of 2 n
// entries, or n where _vectors is NULL; _perm of n, unused where _vectors is
// NULL. Returns whether every eigenvalue was found. If not, those found are
// given and the rest the diagonal's entries, and every entry of _vectors is
// a NaN; a NaN or an infinity in _a gives every eigenvalue and entry a NaN,
// and returns false; the zero matrix has zero eigenvalues and the
// identity's columns.
D_MATH_FN_KERNEL@RT@ bool
d_cmat_@S@_eigen@F@(
    struct dj_cplx_@S@* _a,
    size_t              _n,
    struct dj_cplx_@S@* _values,
    struct dj_cplx_@S@* _vectors,
    struct dj_cplx_@S@* _work,
    @T@*                _rwork,
    size_t*             _perm,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_cmat_@S@_scaling_of(_a, _n * _n);
    const struct dj_cplx_@S@ not_a_number =
        d_cplx_@S@_make(d_internal_math_@S@_nan(), d_internal_math_@S@_nan());
    @T@*                scale     = _rwork;
    struct dj_cplx_@S@* z         = NULL;
    size_t              ilo       = 0u;
    size_t              ihi       = 0u;
    size_t              i         = 0u;
    size_t              k         = 0u;
    bool                converged = true;

    // nothing to find
    if (_n == 0u)
    {
        return true;
    }

    // a NaN or an infinity: no eigenvalue, no eigenvector
    if (!sc.finite)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = not_a_number;
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i] = not_a_number;
            }
        }

        return false;
    }

    // the zero matrix: zeros, and the identity's columns
    if (sc.zero)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = d_cplx_@S@_make((@T@)0, (@T@)0);
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < _n; ++i)
            {
                for (k = 0u; k < _n; ++k)
                {
                    _vectors[(i * _n) + k] =
                        d_cplx_@S@_make((i == k) ? (@T@)1 : (@T@)0, (@T@)0);
                }
            }
        }

        return true;
    }

    // the largest part into [1, 2), exactly
    for (i = 0u; i < (_n * _n); ++i)
    {
        size_t p = 0u;

        for (p = 0u; p < 2u; ++p)
        {
            const @T@ lifted = _a[i].part[p] * sc.pre;

            _a[i].part[p] = lifted / sc.power;
        }
    }

    d_cmat_@S@_balance@F@(_a, _n, _perm, scale, &ilo, &ihi);

    // Q, then Z, from the identity
    if (_vectors != NULL)
    {
        z = _work;

        for (i = 0u; i < _n; ++i)
        {
            for (k = 0u; k < _n; ++k)
            {
                z[(i * _n) + k] =
                    d_cplx_@S@_make((i == k) ? (@T@)1 : (@T@)0, (@T@)0);
            }
        }
    }

    d_cmat_@S@_hessenberg_q@F@(_a, _n, ilo, ihi, z);

    converged = d_cmat_@S@_schur@F@(_a, _n, ilo, ihi, z, _values, _max_iter,
                                    _tol);

    // the eigenvectors, or NaNs where the iteration stopped short
    if (_vectors != NULL)
    {
        if (converged)
        {
            d_cmat_@S@_schur_vectors@F@(_a, _n, z, z + (_n * _n),
                                        _rwork + _n);
            d_cmat_@S@_balance_back(z, _n, _n, _perm, scale, ilo, ihi);

            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i] = z[i];
            }

            d_internal_cmat_@S@_unit_vectors@F@(_vectors, _n);
        }
        else
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i] = not_a_number;
            }
        }
    }

    // back to the matrix's scale: exact unless a value overflows or
    // underflows
    for (i = 0u; i < _n; ++i)
    {
        const @T@ re = _values[i].part[0] * sc.power;
        const @T@ im = _values[i].part[1] * sc.power;

        _values[i].part[0] = re / sc.pre;
        _values[i].part[1] = im / sc.pre;
    }

    // ascending by real part, then imaginary, the vectors' columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;

        for (k = i + 1u; k < _n; ++k)
        {
            const bool lower = (_values[k].part[0] < _values[sel].part[0]);
            const bool level = (_values[k].part[0] == _values[sel].part[0]);
            const bool below = (_values[k].part[1] < _values[sel].part[1]);

            if ( (lower) ||
                 ((level) && (below)) )
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const struct dj_cplx_@S@ tmp = _values[i];

            _values[i]   = _values[sel];
            _values[sel] = tmp;

            if (_vectors != NULL)
            {
                for (k = 0u; k < _n; ++k)
                {
                    const struct dj_cplx_@S@ t = _vectors[(k * _n) + i];

                    _vectors[(k * _n) + i]   = _vectors[(k * _n) + sel];
                    _vectors[(k * _n) + sel] = t;
                }
            }
        }
    }

    return converged;
}
"""

CMAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Products, LU, determinant, inverse and solution
    2.  Hermitian matrices
    3.  The eigenvalues and eigenvectors of a general matrix
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Products, LU, determinant, inverse and solution
    2.  Hermitian matrices
    3.  The eigenvalues and eigenvectors of a general matrix
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Products, LU, determinant, inverse and solution
    2.  Hermitian matrices
    3.  The eigenvalues and eigenvectors of a general matrix
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_CMAT_H
#define DJINTERP_C_MATH_CMAT_H 1

// std
#include <float.h>           // FLT_EPSILON ... LDBL_MAX: the families'
                             // limits
#include <stddef.h>          // size_t
// djinterp
#include "./cplx.h"          // struct dj_cplx_*, its arithmetic
#include "./decomp.h"        // d_internal_decomp_*_hypot, _cnorm and
                             // _scaling: the real helpers
#include "./elementary.h"    // d_math_*_sqrt and its _c form,
                             // d_internal_math_*_abs, d_internal_math_*_nan
#include "./float_order.h"   // d_internal_math_*_binade_of,
                             // d_internal_math_*_pow2
#include "./math_common.h"   // D_MATH_FN, D_MATH_FN_KERNEL
#include "./vec.h"           // d_internal_vec_*_acc_*: the compensated sums
#include "../../../config/c/modules/math/cfg_mat.h"  // D_CFG_MATH_MAT_*
"""

CMAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_CMAT_H
"""

CMAT_OVERLOADS = [
    ("D_MATH_FN_KERNEL", "bool", "d_cmat_lu", [("struct dj_cplx_@S@*", "_lu"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_sign")]),
    ("D_MATH_FN_KERNEL", "void", "d_cmat_lu_solve", [("const struct dj_cplx_@S@*", "_lu"), ("const size_t*", "_perm"), ("size_t", "_n"), ("const struct dj_cplx_@S@*", "_b"), ("struct dj_cplx_@S@*", "_x")]),
    ("D_MATH_FN", "struct dj_cplx_@S@", "d_cmat_lu_det", [("const struct dj_cplx_@S@*", "_lu"), ("size_t", "_n"), ("@T@", "_sign")]),
    ("D_MATH_FN_KERNEL", "struct dj_cplx_@S@", "d_cmat_determinant", [("const struct dj_cplx_@S@*", "_a"), ("struct dj_cplx_@S@*", "_scratch"), ("size_t", "_n")]),
    ("D_MATH_FN_KERNEL", "bool", "d_cmat_inverse", [("struct dj_cplx_@S@*", "_out"), ("struct dj_cplx_@S@*", "_scratch"), ("const struct dj_cplx_@S@*", "_a"), ("size_t", "_n")]),
    ("D_MATH_FN_KERNEL", "bool", "d_cmat_solve_fast", [("struct dj_cplx_@S@*", "_x"), ("struct dj_cplx_@S@*", "_scratch"), ("const struct dj_cplx_@S@*", "_a"), ("const struct dj_cplx_@S@*", "_b"), ("size_t", "_n"), ("size_t", "_m")]),
    ("D_MATH_FN_KERNEL", "bool", "d_cmat_solve", [("struct dj_cplx_@S@*", "_x"), ("struct dj_cplx_@S@*", "_scratch"), ("size_t*", "_pivots"), ("const struct dj_cplx_@S@*", "_a"), ("const struct dj_cplx_@S@*", "_b"), ("size_t", "_n"), ("size_t", "_m")]),
    ("D_MATH_FN_KERNEL", "void", "d_cmat_multiply", [("struct dj_cplx_@S@*", "_out"), ("const struct dj_cplx_@S@*", "_a"), ("const struct dj_cplx_@S@*", "_b"), ("size_t", "_rows"), ("size_t", "_inner"), ("size_t", "_cols")]),
    ("D_MATH_FN_KERNEL", "void", "d_cmat_multiply_fast", [("struct dj_cplx_@S@*", "_out"), ("const struct dj_cplx_@S@*", "_a"), ("const struct dj_cplx_@S@*", "_b"), ("size_t", "_rows"), ("size_t", "_inner"), ("size_t", "_cols")]),
    ("D_MATH_FN_KERNEL", "void", "d_cmat_balance_back", [("struct dj_cplx_@S@*", "_v"), ("size_t", "_n"), ("size_t", "_m"), ("const size_t*", "_perm"), ("const @T@*", "_scale"), ("size_t", "_ilo"), ("size_t", "_ihi")]),
]
for _f, _m in (("_c", "D_MATH_FN_KERNEL"), ("", "D_MATH_FN_KERNEL_RT")):
    CMAT_OVERLOADS += [
        (_m, "void", "d_cmat_hermitian_eigen" + _f, [("struct dj_cplx_@S@*", "_work"), ("@T@*", "_values"), ("struct dj_cplx_@S@*", "_vectors"), ("size_t", "_n"), ("@T@", "_threshold")]),
        (_m, "void", "d_cmat_balance" + _f, [("struct dj_cplx_@S@*", "_a"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_scale"), ("size_t*", "_ilo"), ("size_t*", "_ihi")]),
        (_m, "void", "d_cmat_hessenberg_q" + _f, [("struct dj_cplx_@S@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("struct dj_cplx_@S@*", "_q")]),
        (_m, "bool", "d_cmat_schur" + _f, [("struct dj_cplx_@S@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("struct dj_cplx_@S@*", "_z"), ("struct dj_cplx_@S@*", "_values"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
        (_m, "void", "d_cmat_schur_vectors" + _f, [("const struct dj_cplx_@S@*", "_t"), ("size_t", "_n"), ("struct dj_cplx_@S@*", "_vr"), ("struct dj_cplx_@S@*", "_x"), ("@T@*", "_norms")]),
        (_m, "bool", "d_cmat_eigen" + _f, [("struct dj_cplx_@S@*", "_a"), ("size_t", "_n"), ("struct dj_cplx_@S@*", "_values"), ("struct dj_cplx_@S@*", "_vectors"), ("struct dj_cplx_@S@*", "_work"), ("@T@*", "_rwork"), ("size_t*", "_perm"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
    ]


def cmat():
    lines = [
        "Complex matrices: the C core of complex linear algebra (S269) - LU",
        "with partial pivoting and its solve and determinant, a complex",
        "matrix's determinant, inverse and solution, a Hermitian matrix's",
        "eigenvalues and eigenvectors by Jacobi's method with complex",
        "rotations, and a general complex matrix's by LAPACK's path:",
        "balancing, the Hessenberg form, the complex Schur form by",
        "single-shift QR, the eigenvectors from it - for float, double and",
        "long double, on cplx.h's complex numbers.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Matrices are row-major arrays of struct dj_cplx_f, _d or _ld. What",
        "needs no root comes once, a constant expression from C++14; what",
        "takes a root comes twice, as decomp.h's kernels do, the _c form a",
        "constant expression from C++14 and the plain one on the C library's",
        "root, the same bit for bit, their roots correctly rounded both. The",
        "product is compensated and the solve refined with extra precision, each",
        "with a _fast form as written (S410).",
    ]
    text = banner("cmat.h", lines, "2026.10.29", "2026.11.21")
    text += "\n" + CMAT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        body = CMAT_FAMILY_HEAD + CMAT_LU + CMAT_HERMITIAN_HEAD
        for f, rt in (("_c", ""), ("", "_RT")):
            body += CMAT_HERMITIAN_FORMS.replace("@F@", f).replace("@RT@",
                                                                    rt)
        body += CMAT_GENERAL_HEAD + CMAT_GENERAL_ONCE
        for f, rt in (("_c", ""), ("", "_RT")):
            body += CMAT_GENERAL_FORMS.replace("@F@", f).replace("@RT@", rt)
        body += DECOMP_POP
        body = body.replace("@SECTION@", str(i + 1))
        body = body.replace("@UPPER@", prose.upper())
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)",
                             CMAT_OVERLOADS)
    text += CMAT_TAIL
    return text
