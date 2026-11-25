"""gen_c_math.decomp_text - the text of decomp.h, its first half (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""


# -----------------------------------------------------------------------------
# decomp.h
# -----------------------------------------------------------------------------

DECOMP_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Matrices are a caller's row-major arrays: element (i, j) of an r-by-c
// matrix at [i c + j]. The algorithms are the decomposition header's own,
// in its order, one operation per statement.


// @SECTION@.1    LU, with partial pivoting
//------------------------------------------------------------------------------
// Exact in its arithmetic's order and needing no root: one form, a constant
// expression from C++14.

// d_decomp_@S@_lu
//   factors the n-by-n _lu in place into P A = L U - L's multipliers below
// the diagonal, U on and above - choosing each column's largest pivot (the
// first, among equals). _perm receives the row permutation and *_sign its
// parity (+1 or -1). Returns whether some pivot column was zero, the matrix
// singular; that column is then skipped, U's diagonal zero there.
D_MATH_FN bool
d_decomp_@S@_lu(
    @T@*    _lu,
    size_t  _n,
    size_t* _perm,
    @T@*    _sign
)
{
    bool   singular = false;
    size_t i        = 0u;
    size_t col      = 0u;

    *_sign = (@T@)1;

    for (i = 0u; i < _n; ++i)
    {
        _perm[i] = i;
    }

    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = d_internal_math_@S@_abs(_lu[(col * _n) + col]);
        size_t r     = 0u;

        // the largest magnitude in the column, at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = d_internal_math_@S@_abs(_lu[(r * _n) + col]);

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // a zero column: singular, and nothing to eliminate
        if (_lu[(pivot * _n) + col] == (@T@)0)
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
                const @T@ tmp = _lu[(col * _n) + c];

                _lu[(col * _n) + c]   = _lu[(pivot * _n) + c];
                _lu[(pivot * _n) + c] = tmp;
            }

            {
                const size_t tmp = _perm[col];

                _perm[col]   = _perm[pivot];
                _perm[pivot] = tmp;
            }

            *_sign = -*_sign;
        }

        {
            const @T@ pivot_value = _lu[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const @T@ f = _lu[(r * _n) + col] / pivot_value;
                size_t    c = 0u;

                _lu[(r * _n) + col] = f;

                for (c = col + 1u; c < _n; ++c)
                {
                    const @T@ prod = f * _lu[(col * _n) + c];

                    _lu[(r * _n) + c] = _lu[(r * _n) + c] - prod;
                }
            }
        }
    }

    return singular;
}

// d_decomp_@S@_lu_solve
//   solves A _x = _b from d_decomp_@S@_lu's factors: _b permuted, forward
// through L (unit diagonal), back through U. _x and _b must not overlap.
D_MATH_FN void
d_decomp_@S@_lu_solve(
    const @T@*    _lu,
    const size_t* _perm,
    size_t        _n,
    const @T@*    _b,
    @T@*          _x
)
{
    size_t i = 0u;

    // forward: y into _x
    for (i = 0u; i < _n; ++i)
    {
        @T@    s = _b[_perm[i]];
        size_t k = 0u;

        for (k = 0u; k < i; ++k)
        {
            const @T@ prod = _lu[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s;
    }

    // back: _x over y, last row first
    for (i = _n; i-- > 0u; )
    {
        @T@    s = _x[i];
        size_t k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            const @T@ prod = _lu[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _lu[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_lu_det
//   the determinant from the factors: the parity times U's diagonal, in
// order.
D_MATH_FN @T@
d_decomp_@S@_lu_det(
    const @T@* _lu,
    size_t     _n,
    @T@        _sign
)
{
    @T@    d = _sign;
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        d = d * _lu[(i * _n) + i];
    }

    return d;
}

// d_decomp_@S@_cholesky_solve
//   solves A _x = _b from a Cholesky factor _l (A = L L^T): forward through
// L, back through L^T. _x and _b must not overlap.
D_MATH_FN void
d_decomp_@S@_cholesky_solve(
    const @T@* _l,
    size_t     _n,
    const @T@* _b,
    @T@*       _x
)
{
    size_t i = 0u;

    // forward: y into _x
    for (i = 0u; i < _n; ++i)
    {
        @T@    s = _b[i];
        size_t k = 0u;

        for (k = 0u; k < i; ++k)
        {
            const @T@ prod = _l[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _l[(i * _n) + i];
    }

    // back, through L^T: _x over y, last row first
    for (i = _n; i-- > 0u; )
    {
        @T@    s = _x[i];
        size_t k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            const @T@ prod = _l[(k * _n) + i] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _l[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_cholesky_det
//   the determinant from a Cholesky factor: L's diagonal product, squared.
D_MATH_FN @T@
d_decomp_@S@_cholesky_det(
    const @T@* _l,
    size_t     _n
)
{
    @T@    d = (@T@)1;
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        d = d * _l[(i * _n) + i];
    }

    return d * d;
}

// d_decomp_@S@_forward_sub
//   solves L _x = _b for the n-by-n lower triangle of _l (its diagonal
// general, not unit), row by row. _x and _b must not overlap.
D_MATH_FN void
d_decomp_@S@_forward_sub(
    const @T@* _l,
    size_t     _n,
    const @T@* _b,
    @T@*       _x
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        @T@    s = _b[i];
        size_t k = 0u;

        for (k = 0u; k < i; ++k)
        {
            const @T@ prod = _l[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _l[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_back_sub
//   solves U _x = _b for the n-by-n upper triangle of _u (its diagonal
// general), last row first. _u may be the first n rows of a taller
// row-major matrix of n columns, as an R from QR is. _x and _b must not
// overlap.
D_MATH_FN void
d_decomp_@S@_back_sub(
    const @T@* _u,
    size_t     _n,
    const @T@* _b,
    @T@*       _x
)
{
    size_t i = 0u;

    for (i = _n; i-- > 0u; )
    {
        @T@    s = _b[i];
        size_t k = 0u;

        for (k = i + 1u; k < _n; ++k)
        {
            const @T@ prod = _u[(i * _n) + k] * _x[k];

            s = s - prod;
        }

        _x[i] = s / _u[(i * _n) + i];
    }

    return;
}

// d_decomp_@S@_qt_mul
//   _c = (Q^T _b)[0:n] for the m-by-m _q: each of the first n columns of Q
// against _b, summed over the rows in order.
D_MATH_FN void
d_decomp_@S@_qt_mul(
    const @T@* _q,
    size_t     _m,
    size_t     _n,
    const @T@* _b,
    @T@*       _c
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        @T@    s = (@T@)0;
        size_t k = 0u;

        for (k = 0u; k < _m; ++k)
        {
            const @T@ prod = _q[(k * _m) + i] * _b[k];

            s = s + prod;
        }

        _c[i] = s;
    }

    return;
}

// @SECTION@.2    QR, the SVD, the symmetric eigensolver and Cholesky
//------------------------------------------------------------------------------
// Each takes square roots, and comes twice: the _c form a constant
// expression from C++14, the plain one on the C library's root.
@FORMS@
// @SECTION@.3    The eigenvalues and eigenvectors of a general matrix
//------------------------------------------------------------------------------
// LAPACK's path, as its dgeev takes it (S269): the matrix scaled by a power
// of two so that its largest magnitude lies in [1, 2), balanced (dgebal),
// reduced to upper Hessenberg form by Householder reflectors (dgehd2), its
// real Schur form found by Francis's implicit double-shift QR (dlahqr), its
// blocks of two brought to standard form (dlanv2), so that each complex
// pair is exactly conjugate and each real eigenvalue's imaginary part
// exactly +0; then the eigenvectors from the Schur form by back-substitution
// (dtrevc), back-transformed (dgebak). The eigenvalues are cplx.h's complex
// numbers, ascending by real part, then by imaginary part. The scaling is
// exact, so 2^k A has 2^k times A's eigenvalues, bit for bit, barring
// overflow and underflow. The rotation Hessenberg form and an eigenvector
// by inverse iteration, for one eigenvalue, stay beside them. What takes a
// root comes twice, as above: the two forms the same bit for bit, as their
// roots are correctly rounded both.

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

// dj_internal_decomp_@S@_rotation
//   struct: a plane rotation and the length it leaves: c x + s y = r and
// c y - s x = 0, for the pair (x, y) it was made from.
struct dj_internal_decomp_@S@_rotation
{
    @T@ c;  // the cosine
    @T@ s;  // the sine
    @T@ r;  // the pair's length, sqrt(x^2 + y^2) >= 0
};

// dj_internal_decomp_@S@_scaling
//   struct: how a matrix is brought to a largest magnitude in [1, 2): times
// pre, then divided by power, both powers of two, so both steps are exact
// (but for an entry that underflows, which is below the largest's rounding).
struct dj_internal_decomp_@S@_scaling
{
    @T@  pre;     // 2^P for a subnormal largest magnitude, else 1
    @T@  power;   // the largest magnitude's binade, 2^E, times pre
    bool finite;  // whether every entry is finite
    bool zero;    // whether every entry is zero
};

// d_internal_decomp_@S@_scaling_of
//   the scaling of the _count entries of _a: their largest magnitude's
// binade, found exactly (float_order.h), after a lift by 2^P where that
// magnitude is subnormal. Not finite, or zero, when an entry is NaN or
// infinite, or every entry is zero; pre and power are then 1.
D_MATH_FN struct dj_internal_decomp_@S@_scaling
d_internal_decomp_@S@_scaling_of(
    const @T@* _a,
    size_t     _count
)
{
    struct dj_internal_decomp_@S@_scaling result =
        { (@T@)1, (@T@)1, true, false };
    @T@    largest = (@T@)0;
    size_t i       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ v = d_internal_math_@S@_abs(_a[i]);

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

    // every entry zero
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

// d_decomp_@S@_balance_back
//   undoes d_decomp_@S@_balance on the n-by-_m row-major _v, whose columns
// are right eigenvectors of the balanced matrix, as LAPACK's dgebak does:
// each row from _ilo to _ihi multiplied by its scaling factor, exactly, then
// the permutations undone, the last first - the columns pushed to the top,
// then the rows pushed to the bottom - each a swap of two rows. The columns
// are then eigenvectors of the matrix as it was.
D_MATH_FN_KERNEL void
d_decomp_@S@_balance_back(
    @T@*          _v,
    size_t        _n,
    size_t        _m,
    const size_t* _perm,
    const @T@*    _scale,
    size_t        _ilo,
    size_t        _ihi
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
            _v[(i * _m) + c] = _v[(i * _m) + c] * _scale[i];
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
            const @T@ tmp = _v[(row * _m) + c];

            _v[(row * _m) + c]   = _v[(other * _m) + c];
            _v[(other * _m) + c] = tmp;
        }
    }

    return;
}

// d_internal_decomp_@S@_swap_isolated
//   the swap balancing makes to isolate an eigenvalue: columns _i and _j in
// rows 0 to _l, then rows _i and _j in columns _k to n - 1 - a similarity
// by a permutation.
D_MATH_FN void
d_internal_decomp_@S@_swap_isolated(
    @T@*   _a,
    size_t _n,
    size_t _i,
    size_t _j,
    size_t _k,
    size_t _l
)
{
    size_t r = 0u;

    for (r = 0u; r <= _l; ++r)
    {
        const @T@ tmp = _a[(r * _n) + _i];

        _a[(r * _n) + _i] = _a[(r * _n) + _j];
        _a[(r * _n) + _j] = tmp;
    }

    for (r = _k; r < _n; ++r)
    {
        const @T@ tmp = _a[(_i * _n) + r];

        _a[(_i * _n) + r] = _a[(_j * _n) + r];
        _a[(_j * _n) + r] = tmp;
    }

    return;
}

// d_internal_decomp_@S@_small_solve
//   solves the 1-by-1 or 2-by-2 system (T - w I) X = scale B, as LAPACK's
// dlaln2 does for dtrevc's right eigenvectors: T the block of _na rows at
// _t (row stride _n), w = _wr, or _wr + _wi i where _nw is 2, B's real
// parts in _br and its imaginary ones in _bi, X likewise into _xr and _xi.
// A pivot smaller than _smin is taken as _smin, so that a nearly singular
// system perturbs rather than fails; *_scale, at most 1, is chosen so that
// X does not overflow, and *_xnorm receives X's largest magnitude (|re| +
// |im| where complex). Gaussian elimination with complete pivoting for 2 by
// 2; complex quotients are cplx.h's.
D_MATH_FN_KERNEL void
d_internal_decomp_@S@_small_solve(
    size_t     _na,
    size_t     _nw,
    @T@        _smin,
    const @T@* _t,
    size_t     _n,
    const @T@* _br,
    const @T@* _bi,
    @T@        _wr,
    @T@        _wi,
    @T@*       _xr,
    @T@*       _xi,
    @T@*       _scale,
    @T@*       _xnorm
)
{
    // the pivots' orders for each choice of largest entry, by columns: an
    // entry's place in (c11, c21, c12, c22) after the pivot is moved to c11
    const size_t pivots[4][4] = {
        { 0u, 1u, 2u, 3u },
        { 1u, 0u, 3u, 2u },
        { 2u, 3u, 0u, 1u },
        { 3u, 2u, 1u, 0u }
    };
    const bool   rswap[4]     = { false, true, false, true };
    const bool   zswap[4]     = { false, false, true, true };
    const @T@ smlnum = (@T@)2 * @MIN@;
    const @T@ bignum = (@T@)1 / smlnum;
    const @T@ smini  = (_smin > smlnum) ? _smin : smlnum;

    *_scale = (@T@)1;

    // 1 by 1, real: x = b / (t - wr)
    if ( (_na == 1u) &&
         (_nw == 1u) )
    {
        @T@       csr   = _t[0] - _wr;
        @T@       cnorm = d_internal_math_@S@_abs(csr);
        const @T@ bnorm = d_internal_math_@S@_abs(_br[0]);

        // a pivot below smin: smin
        if (cnorm < smini)
        {
            csr   = smini;
            cnorm = smini;
        }

        // b / c would overflow: scaled
        if ( (cnorm < (@T@)1) &&
             (bnorm > (@T@)1) )
        {
            const @T@ bound = bignum * cnorm;

            if (bnorm > bound)
            {
                *_scale = (@T@)1 / bnorm;
            }
        }

        {
            const @T@ scaled = _br[0] * *_scale;

            _xr[0] = scaled / csr;
        }

        *_xnorm = d_internal_math_@S@_abs(_xr[0]);

        return;
    }

    // 1 by 1, complex: x = b / (t - wr - wi i)
    if (_na == 1u)
    {
        @T@       csr   = _t[0] - _wr;
        @T@       csi   = -_wi;
        const @T@ c1    = d_internal_math_@S@_abs(csr);
        const @T@ c2    = d_internal_math_@S@_abs(csi);
        @T@       cnorm = c1 + c2;
        const @T@ b1    = d_internal_math_@S@_abs(_br[0]);
        const @T@ b2    = d_internal_math_@S@_abs(_bi[0]);
        const @T@ bnorm = b1 + b2;

        // a pivot below smin: smin
        if (cnorm < smini)
        {
            csr   = smini;
            csi   = (@T@)0;
            cnorm = smini;
        }

        // b / c would overflow: scaled
        if ( (cnorm < (@T@)1) &&
             (bnorm > (@T@)1) )
        {
            const @T@ bound = bignum * cnorm;

            if (bnorm > bound)
            {
                *_scale = (@T@)1 / bnorm;
            }
        }

        {
            const @T@ sr = *_scale * _br[0];
            const @T@ si = *_scale * _bi[0];
            const struct dj_cplx_@S@ q =
                d_cplx_@S@_div(d_cplx_@S@_make(sr, si),
                               d_cplx_@S@_make(csr, csi));
            const @T@ x1 = d_internal_math_@S@_abs(q.part[0]);
            const @T@ x2 = d_internal_math_@S@_abs(q.part[1]);

            _xr[0]  = q.part[0];
            _xi[0]  = q.part[1];
            *_xnorm = x1 + x2;
        }

        return;
    }

    {
        // C = T - w I, by columns: c11, c21, c12, c22
        const @T@ crv[4] = { _t[0] - _wr, _t[_n], _t[1], _t[_n + 1u] - _wr };
        const @T@ civ[4] = { -_wi, (@T@)0, (@T@)0, -_wi };
        @T@       cmax   = (@T@)0;
        size_t    icmax  = 0u;
        size_t    j      = 0u;

        // the largest entry, |re| + |im|: the first among equals
        for (j = 0u; j < 4u; ++j)
        {
            const @T@ m1 = d_internal_math_@S@_abs(crv[j]);
            const @T@ m2 = (_nw == 2u) ? d_internal_math_@S@_abs(civ[j])
                                       : (@T@)0;
            const @T@ m  = m1 + m2;

            if (m > cmax)
            {
                cmax  = m;
                icmax = j;
            }
        }

        // C below smin: smin times the identity
        if (cmax < smini)
        {
            const @T@ r1    = d_internal_math_@S@_abs(_br[0]);
            const @T@ r2    = d_internal_math_@S@_abs(_br[1]);
            const @T@ i1    = (_nw == 2u) ? d_internal_math_@S@_abs(_bi[0])
                                          : (@T@)0;
            const @T@ i2    = (_nw == 2u) ? d_internal_math_@S@_abs(_bi[1])
                                          : (@T@)0;
            const @T@ s1    = r1 + i1;
            const @T@ s2    = r2 + i2;
            const @T@ bnorm = (s1 > s2) ? s1 : s2;
            @T@       temp  = (@T@)0;

            if ( (smini < (@T@)1) &&
                 (bnorm > (@T@)1) )
            {
                const @T@ bound = bignum * smini;

                if (bnorm > bound)
                {
                    *_scale = (@T@)1 / bnorm;
                }
            }

            temp   = *_scale / smini;
            _xr[0] = temp * _br[0];
            _xr[1] = temp * _br[1];

            if (_nw == 2u)
            {
                _xi[0] = temp * _bi[0];
                _xi[1] = temp * _bi[1];
            }

            *_xnorm = temp * bnorm;

            return;
        }

        // real: elimination with complete pivoting
        if (_nw == 1u)
        {
            const @T@ ur11  = crv[icmax];
            const @T@ cr21  = crv[pivots[icmax][1]];
            const @T@ ur12  = crv[pivots[icmax][2]];
            const @T@ cr22  = crv[pivots[icmax][3]];
            const @T@ ur11r = (@T@)1 / ur11;
            const @T@ lr21  = ur11r * cr21;
            const @T@ cross = ur12 * lr21;
            @T@       ur22  = cr22 - cross;
            @T@       br1   = rswap[icmax] ? _br[1] : _br[0];
            @T@       br2   = rswap[icmax] ? _br[0] : _br[1];
            @T@       bbnd  = (@T@)0;
            @T@       xr1   = (@T@)0;
            @T@       xr2   = (@T@)0;

            // the second pivot below smin: smin
            if (d_internal_math_@S@_abs(ur22) < smini)
            {
                ur22 = smini;
            }

            {
                const @T@ l = lr21 * br1;

                br2 = br2 - l;
            }

            {
                const @T@ ratio = ur22 * ur11r;
                const @T@ p     = br1 * ratio;
                const @T@ a1    = d_internal_math_@S@_abs(p);
                const @T@ a2    = d_internal_math_@S@_abs(br2);

                bbnd = (a1 > a2) ? a1 : a2;
            }

            // the solution would overflow: scaled
            if ( (bbnd > (@T@)1) &&
                 (d_internal_math_@S@_abs(ur22) < (@T@)1) )
            {
                const @T@ bound = bignum * d_internal_math_@S@_abs(ur22);

                if (bbnd >= bound)
                {
                    *_scale = (@T@)1 / bbnd;
                }
            }

            {
                const @T@ s2 = br2 * *_scale;
                const @T@ s1 = *_scale * br1;
                const @T@ f1 = s1 * ur11r;
                const @T@ f2 = ur11r * ur12;

                xr2 = s2 / ur22;

                {
                    const @T@ g = xr2 * f2;

                    xr1 = f1 - g;
                }
            }

            _xr[0] = zswap[icmax] ? xr2 : xr1;
            _xr[1] = zswap[icmax] ? xr1 : xr2;

            {
                const @T@ a1 = d_internal_math_@S@_abs(xr1);
                const @T@ a2 = d_internal_math_@S@_abs(xr2);

                *_xnorm = (a1 > a2) ? a1 : a2;
            }

            // |C| |X| would overflow: scaled again
            if ( (*_xnorm > (@T@)1) &&
                 (cmax > (@T@)1) )
            {
                const @T@ bound = bignum / cmax;

                if (*_xnorm > bound)
                {
                    const @T@ temp = cmax / bignum;

                    _xr[0]  = temp * _xr[0];
                    _xr[1]  = temp * _xr[1];
                    *_xnorm = temp * *_xnorm;
                    *_scale = temp * *_scale;
                }
            }

            return;
        }

        // complex: elimination with complete pivoting
        {
            const @T@ ur11  = crv[icmax];
            const @T@ ui11  = civ[icmax];
            const @T@ cr21  = crv[pivots[icmax][1]];
            const @T@ ci21  = civ[pivots[icmax][1]];
            const @T@ ur12  = crv[pivots[icmax][2]];
            const @T@ ui12  = civ[pivots[icmax][2]];
            const @T@ cr22  = crv[pivots[icmax][3]];
            const @T@ ci22  = civ[pivots[icmax][3]];
            @T@       ur11r = (@T@)0;
            @T@       ui11r = (@T@)0;
            @T@       lr21  = (@T@)0;
            @T@       li21  = (@T@)0;
            @T@       ur12s = (@T@)0;
            @T@       ui12s = (@T@)0;
            @T@       ur22  = (@T@)0;
            @T@       ui22  = (@T@)0;
            @T@       u22abs = (@T@)0;
            @T@       br1   = rswap[icmax] ? _br[1] : _br[0];
            @T@       br2   = rswap[icmax] ? _br[0] : _br[1];
            @T@       bi1   = rswap[icmax] ? _bi[1] : _bi[0];
            @T@       bi2   = rswap[icmax] ? _bi[0] : _bi[1];
            @T@       bbnd  = (@T@)0;
            @T@       xr1   = (@T@)0;
            @T@       xi1   = (@T@)0;
            @T@       xr2   = (@T@)0;
            @T@       xi2   = (@T@)0;

            // the pivot on the diagonal: its off-diagonal partners real
            if ( (icmax == 0u) ||
                 (icmax == 3u) )
            {
                // 1 / (ur11 + ui11 i), the larger part divided out first
                if (d_internal_math_@S@_abs(ur11) >
                    d_internal_math_@S@_abs(ui11))
                {
                    const @T@ temp = ui11 / ur11;
                    const @T@ tt   = temp * temp;
                    const @T@ one  = (@T@)1 + tt;
                    const @T@ den  = ur11 * one;

                    ur11r = (@T@)1 / den;
                    ui11r = -temp * ur11r;
                }
                else
                {
                    const @T@ temp = ur11 / ui11;
                    const @T@ tt   = temp * temp;
                    const @T@ one  = (@T@)1 + tt;
                    const @T@ den  = ui11 * one;

                    ui11r = (@T@)-1 / den;
                    ur11r = -temp * ui11r;
                }

                lr21  = cr21 * ur11r;
                li21  = cr21 * ui11r;
                ur12s = ur12 * ur11r;
                ui12s = ur12 * ui11r;

                {
                    const @T@ p1 = ur12 * lr21;
                    const @T@ p2 = ur12 * li21;

                    ur22 = cr22 - p1;
                    ui22 = ci22 - p2;
                }
            }
            // the pivot off the diagonal: the diagonal real
            else
            {
                ur11r = (@T@)1 / ur11;
                ui11r = (@T@)0;
                lr21  = cr21 * ur11r;
                li21  = ci21 * ur11r;
                ur12s = ur12 * ur11r;
                ui12s = ui12 * ur11r;

                {
                    const @T@ p1 = ur12 * lr21;
                    const @T@ p2 = ui12 * li21;
                    const @T@ p3 = ur12 * li21;
                    const @T@ p4 = ui12 * lr21;
                    const @T@ d1 = cr22 - p1;
                    const @T@ m3 = -p3;

                    ur22 = d1 + p2;
                    ui22 = m3 - p4;
                }
            }

            {
                const @T@ a1 = d_internal_math_@S@_abs(ur22);
                const @T@ a2 = d_internal_math_@S@_abs(ui22);

                u22abs = a1 + a2;
            }

            // the second pivot below smin: smin
            if (u22abs < smini)
            {
                ur22   = smini;
                ui22   = (@T@)0;
            }

            {
                const @T@ p1 = lr21 * br1;
                const @T@ p2 = li21 * bi1;
                const @T@ p3 = li21 * br1;
                const @T@ p4 = lr21 * bi1;
                const @T@ d1 = br2 - p1;
                const @T@ d2 = bi2 - p3;

                br2 = d1 + p2;
                bi2 = d2 - p4;
            }

            {
                const @T@ a1  = d_internal_math_@S@_abs(br1);
                const @T@ a2  = d_internal_math_@S@_abs(bi1);
                const @T@ a3  = d_internal_math_@S@_abs(ur11r);
                const @T@ a4  = d_internal_math_@S@_abs(ui11r);
                const @T@ a5  = d_internal_math_@S@_abs(br2);
                const @T@ a6  = d_internal_math_@S@_abs(bi2);
                const @T@ s12 = a1 + a2;
                const @T@ s34 = a3 + a4;
                const @T@ u   = u22abs * s34;
                const @T@ e1  = s12 * u;
                const @T@ e2  = a5 + a6;

                bbnd = (e1 > e2) ? e1 : e2;
            }

            // the solution would overflow: scaled
            if ( (bbnd > (@T@)1) &&
                 (u22abs < (@T@)1) )
            {
                const @T@ bound = bignum * u22abs;

                if (bbnd >= bound)
                {
                    *_scale = (@T@)1 / bbnd;
                    br1     = *_scale * br1;
                    bi1     = *_scale * bi1;
                    br2     = *_scale * br2;
                    bi2     = *_scale * bi2;
                }
            }

            {
                const struct dj_cplx_@S@ q =
                    d_cplx_@S@_div(d_cplx_@S@_make(br2, bi2),
                                   d_cplx_@S@_make(ur22, ui22));

                xr2 = q.part[0];
                xi2 = q.part[1];
            }

            {
                const @T@ p1 = ur11r * br1;
                const @T@ p2 = ui11r * bi1;
                const @T@ p3 = ur12s * xr2;
                const @T@ p4 = ui12s * xi2;
                const @T@ d1 = p1 - p2;
                const @T@ d2 = d1 - p3;

                xr1 = d2 + p4;
            }

            {
                const @T@ p1 = ui11r * br1;
                const @T@ p2 = ur11r * bi1;
                const @T@ p3 = ui12s * xr2;
                const @T@ p4 = ur12s * xi2;
                const @T@ s1 = p1 + p2;
                const @T@ d2 = s1 - p3;

                xi1 = d2 - p4;
            }

            _xr[0] = zswap[icmax] ? xr2 : xr1;
            _xr[1] = zswap[icmax] ? xr1 : xr2;
            _xi[0] = zswap[icmax] ? xi2 : xi1;
            _xi[1] = zswap[icmax] ? xi1 : xi2;

            {
                const @T@ a1 = d_internal_math_@S@_abs(xr1);
                const @T@ a2 = d_internal_math_@S@_abs(xi1);
                const @T@ a3 = d_internal_math_@S@_abs(xr2);
                const @T@ a4 = d_internal_math_@S@_abs(xi2);
                const @T@ s1 = a1 + a2;
                const @T@ s2 = a3 + a4;

                *_xnorm = (s1 > s2) ? s1 : s2;
            }

            // |C| |X| would overflow: scaled again
            if ( (*_xnorm > (@T@)1) &&
                 (cmax > (@T@)1) )
            {
                const @T@ bound = bignum / cmax;

                if (*_xnorm > bound)
                {
                    const @T@ temp = cmax / bignum;

                    _xr[0]  = temp * _xr[0];
                    _xr[1]  = temp * _xr[1];
                    _xi[0]  = temp * _xi[0];
                    _xi[1]  = temp * _xi[1];
                    *_xnorm = temp * *_xnorm;
                    *_scale = temp * *_scale;
                }
            }
        }
    }

    return;
}

// d_internal_decomp_@S@_put
//   eigenvalue _j, _re + _im i, put where the caller asked: into _values[_j]
// unless _values is NULL, its real part into _real[_j] unless _real is NULL,
// and *_complex set where _im is not zero, unless _complex is NULL.
D_MATH_FN void
d_internal_decomp_@S@_put(
    struct dj_cplx_@S@* _values,
    @T@*                _real,
    bool*               _complex,
    size_t              _j,
    @T@                 _re,
    @T@                 _im
)
{
    if (_values != NULL)
    {
        _values[_j].part[0] = _re;
        _values[_j].part[1] = _im;
    }

    if (_real != NULL)
    {
        _real[_j] = _re;
    }

    if ( (_complex != NULL) &&
         (_im != (@T@)0) )
    {
        *_complex = true;
    }

    return;
}

// d_internal_decomp_@S@_reflect3
//   applies the reflector I - t1 (1, v2, v3)(1, v2, v3)^T, t2 = t1 v2 and
// t3 = t1 v3, to the three entries at _x, _x + _step and _x + 2 _step, or
// with _three false the reflector of (1, v2) to the first two: each entry
// less its share of their sum, as LAPACK's dlahqr writes it.
D_MATH_FN void
d_internal_decomp_@S@_reflect3(
    @T@*   _x,
    size_t _step,
    bool   _three,
    @T@    _v2,
    @T@    _v3,
    @T@    _t1,
    @T@    _t2,
    @T@    _t3
)
{
    const @T@ p2  = _v2 * _x[_step];
    @T@       sum = _x[0] + p2;

    if (_three)
    {
        const @T@ p3 = _v3 * _x[2u * _step];

        sum = sum + p3;
    }

    {
        const @T@ s1 = sum * _t1;
        const @T@ s2 = sum * _t2;

        _x[0]     = _x[0] - s1;
        _x[_step] = _x[_step] - s2;
    }

    if (_three)
    {
        const @T@ s3 = sum * _t3;

        _x[2u * _step] = _x[2u * _step] - s3;
    }

    return;
}

// d_internal_decomp_@S@_rotate
//   the plane rotation (x, y) -> (cs x + sn y, cs y - sn x) of the _count
// pairs _x[0], _y[0], _x[_stride], _y[_stride], ..., as BLAS's drot.
D_MATH_FN void
d_internal_decomp_@S@_rotate(
    @T@*   _x,
    @T@*   _y,
    size_t _count,
    size_t _stride,
    @T@    _cs,
    @T@    _sn
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ x  = _x[i * _stride];
        const @T@ y  = _y[i * _stride];
        const @T@ a1 = _cs * x;
        const @T@ a2 = _sn * y;
        const @T@ b1 = _cs * y;
        const @T@ b2 = _sn * x;

        _x[i * _stride] = a1 + a2;
        _y[i * _stride] = b1 - b2;
    }

    return;
}

@EIGEN_FORMS@"""

DECOMP_FORMS = r"""
// d_decomp_@S@_qr@F@
//   Householder QR of the m-by-n _r (m >= n) in place, A = Q R: _q, the
// m-by-m identity on entry, receives Q, and _r becomes R. _v is scratch of m
// elements. A column already zero below the diagonal is left as it is.
D_MATH_FN@RT@ void
d_decomp_@S@_qr@F@(
    @T@*   _q,
    @T@*   _r,
    size_t _m,
    size_t _n,
    @T@*   _v
)
{
    size_t k = 0u;

    for (k = 0u; k < _n; ++k)
    {
        @T@    norm_sq = (@T@)0;
        @T@    normx   = (@T@)0;
        @T@    vtv     = (@T@)0;
        size_t i       = 0u;
        size_t j       = 0u;

        for (i = k; i < _m; ++i)
        {
            _v[i] = _r[(i * _n) + k];
        }

        for (i = k; i < _m; ++i)
        {
            const @T@ sq = _v[i] * _v[i];

            norm_sq = norm_sq + sq;
        }

        normx = d_math_@S@_sqrt@F@(norm_sq);

        // nothing below the diagonal to reflect away
        if (normx == (@T@)0)
        {
            continue;
        }

        {
            const @T@ alpha = (_v[k] >= (@T@)0) ? -normx : normx;

            _v[k] = _v[k] - alpha;
        }

        for (i = k; i < _m; ++i)
        {
            const @T@ sq = _v[i] * _v[i];

            vtv = vtv + sq;
        }

        // a degenerate reflector: leave the column
        if (vtv == (@T@)0)
        {
            continue;
        }

        {
            const @T@ two_over = (@T@)2 / vtv;
            size_t    row      = 0u;

            // R <- H R, column by column
            for (j = 0u; j < _n; ++j)
            {
                @T@ s = (@T@)0;

                for (i = k; i < _m; ++i)
                {
                    const @T@ prod = _v[i] * _r[(i * _n) + j];

                    s = s + prod;
                }

                {
                    const @T@ f = two_over * s;

                    for (i = k; i < _m; ++i)
                    {
                        const @T@ prod = f * _v[i];

                        _r[(i * _n) + j] = _r[(i * _n) + j] - prod;
                    }
                }
            }

            // Q <- Q H, row by row
            for (row = 0u; row < _m; ++row)
            {
                @T@ s = (@T@)0;

                for (i = k; i < _m; ++i)
                {
                    const @T@ prod = _q[(row * _m) + i] * _v[i];

                    s = s + prod;
                }

                {
                    const @T@ f = two_over * s;

                    for (i = k; i < _m; ++i)
                    {
                        const @T@ prod = f * _v[i];

                        _q[(row * _m) + i] = _q[(row * _m) + i] - prod;
                    }
                }
            }
        }
    }

    return;
}

// d_decomp_@S@_svd@F@
//   the thin SVD of the m-by-n _work (m >= n), A = U S V^T, by one-sided
// (Hestenes) Jacobi: column pairs rotated until each pair's correlation
// |gamma| is within _tol of the product of their norms, at most 60 sweeps.
// _work is consumed; _v, the n-by-n identity on entry, receives V; _s the
// singular values, descending (the first largest, among equals); _u the
// normalized columns, a zero one for a zero singular value. U's and V's
// columns follow their values in the sort.
D_MATH_FN@RT@ void
d_decomp_@S@_svd@F@(
    @T@*   _work,
    @T@*   _u,
    @T@*   _s,
    @T@*   _v,
    size_t _m,
    size_t _n,
    @T@    _tol
)
{
    size_t sweep = 0u;
    size_t i     = 0u;
    size_t j     = 0u;
    size_t k     = 0u;

    for (sweep = 0u; sweep < 60u; ++sweep)
    {
        bool rotated = false;

        for (i = 0u; i < _n; ++i)
        {
            for (j = i + 1u; j < _n; ++j)
            {
                @T@ alpha = (@T@)0;
                @T@ beta  = (@T@)0;
                @T@ gamma = (@T@)0;

                for (k = 0u; k < _m; ++k)
                {
                    const @T@ wki = _work[(k * _n) + i];
                    const @T@ wkj = _work[(k * _n) + j];
                    const @T@ aa  = wki * wki;
                    const @T@ bb  = wkj * wkj;
                    const @T@ ab  = wki * wkj;

                    alpha = alpha + aa;
                    beta  = beta + bb;
                    gamma = gamma + ab;
                }

                {
                    const @T@ ab_sq   = alpha * beta;
                    const @T@ product = d_math_@S@_sqrt@F@(ab_sq);
                    const @T@ bound   = _tol * product;

                    // the pair is orthogonal enough: no rotation
                    if (!(d_internal_math_@S@_abs(gamma) > bound))
                    {
                        continue;
                    }
                }

                rotated = true;

                {
                    const @T@ diff  = beta - alpha;
                    const @T@ twog  = (@T@)2 * gamma;
                    const @T@ half  = d_internal_math_@S@_pow2(@EMAX@ / 2);
                    const @T@ cut   = d_internal_math_@S@_abs(diff) / half;
                    const bool   far   = (cut >= d_internal_math_@S@_abs(twog));
                    const @T@ zeta  = far ? (@T@)0 : (diff / twog);
                    @T@       t     = (@T@)1;
                    @T@       c     = (@T@)0;
                    @T@       s     = (@T@)0;

                    // the smaller root of t^2 + 2 zeta t - 1 = 0; past
                    // 2^(EMAX / 2), where zeta^2 would overflow, 1 / (2
                    // zeta), gamma / diff, which it equals within a part in
                    // 2^EMAX
                    if ( (far) ||
                         (d_internal_math_@S@_abs(zeta) >= half) )
                    {
                        t = gamma / diff;
                    }
                    else if (zeta != (@T@)0)
                    {
                        const @T@ sgn  = (zeta > (@T@)0) ? (@T@)1
                                                         : (@T@)-1;
                        const @T@ zz   = zeta * zeta;
                        const @T@ one  = (@T@)1 + zz;
                        const @T@ root = d_math_@S@_sqrt@F@(one);
                        const @T@ mag  = d_internal_math_@S@_abs(zeta);
                        const @T@ den  = mag + root;

                        t = sgn / den;
                    }

                    {
                        const @T@ tt   = t * t;
                        const @T@ one  = (@T@)1 + tt;
                        const @T@ root = d_math_@S@_sqrt@F@(one);

                        c = (@T@)1 / root;
                        s = c * t;
                    }

                    // rotate the columns of the work matrix, then of V
                    for (k = 0u; k < _m; ++k)
                    {
                        const @T@ wki = _work[(k * _n) + i];
                        const @T@ wkj = _work[(k * _n) + j];
                        const @T@ a1  = c * wki;
                        const @T@ a2  = s * wkj;
                        const @T@ b1  = s * wki;
                        const @T@ b2  = c * wkj;

                        _work[(k * _n) + i] = a1 - a2;
                        _work[(k * _n) + j] = b1 + b2;
                    }

                    for (k = 0u; k < _n; ++k)
                    {
                        const @T@ vki = _v[(k * _n) + i];
                        const @T@ vkj = _v[(k * _n) + j];
                        const @T@ a1  = c * vki;
                        const @T@ a2  = s * vkj;
                        const @T@ b1  = s * vki;
                        const @T@ b2  = c * vkj;

                        _v[(k * _n) + i] = a1 - a2;
                        _v[(k * _n) + j] = b1 + b2;
                    }
                }
            }
        }

        // a sweep without a rotation: converged
        if (!rotated)
        {
            break;
        }
    }

    // the singular values and U, column by column
    for (j = 0u; j < _n; ++j)
    {
        @T@ norm_sq = (@T@)0;
        @T@ sigma   = (@T@)0;

        for (k = 0u; k < _m; ++k)
        {
            const @T@ sq = _work[(k * _n) + j] * _work[(k * _n) + j];

            norm_sq = norm_sq + sq;
        }

        sigma  = d_math_@S@_sqrt@F@(norm_sq);
        _s[j]  = sigma;

        for (k = 0u; k < _m; ++k)
        {
            const @T@ w = _work[(k * _n) + j];

            _u[(k * _n) + j] = (sigma > (@T@)0) ? (w / sigma) : (@T@)0;
        }
    }

    // descending, U's and V's columns along
    for (i = 0u; i < _n; ++i)
    {
        size_t sel = i;

        for (k = i + 1u; k < _n; ++k)
        {
            if (_s[k] > _s[sel])
            {
                sel = k;
            }
        }

        if (sel != i)
        {
            const @T@ tmp = _s[i];

            _s[i]   = _s[sel];
            _s[sel] = tmp;

            for (k = 0u; k < _m; ++k)
            {
                const @T@ t = _u[(k * _n) + i];

                _u[(k * _n) + i]   = _u[(k * _n) + sel];
                _u[(k * _n) + sel] = t;
            }

            for (k = 0u; k < _n; ++k)
            {
                const @T@ t = _v[(k * _n) + i];

                _v[(k * _n) + i]   = _v[(k * _n) + sel];
                _v[(k * _n) + sel] = t;
            }
        }
    }

    return;
}

// d_decomp_@S@_sym_eigen@F@
//   the eigenvalues and eigenvectors of the symmetric n-by-n _work by the
// cyclic Jacobi method: sweeps of plane rotations, each annihilating an
// off-diagonal pair, until the off-diagonal norm is within _threshold, at
// most 100 sweeps. _work is consumed; _vectors, the n-by-n identity on
// entry, receives the eigenvectors as columns; _values the eigenvalues,
// ascending (the first smallest, among equals), the columns following.
D_MATH_FN@RT@ void
d_decomp_@S@_sym_eigen@F@(
    @T@*   _work,
    @T@*   _values,
    @T@*   _vectors,
    size_t _n,
    @T@    _threshold
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
                const @T@ sq = _work[(p * _n) + q] * _work[(p * _n) + q];

                off_sq = off_sq + sq;
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
                const @T@ apq = _work[(p * _n) + q];

                // already zero: nothing to annihilate
                if (apq == (@T@)0)
                {
                    continue;
                }

                {
                    const @T@ aqq   = _work[(q * _n) + q];
                    const @T@ app   = _work[(p * _n) + p];
                    const @T@ diff  = aqq - app;
                    const @T@ twoa  = (@T@)2 * apq;
                    const @T@ half  = d_internal_math_@S@_pow2(@EMAX@ / 2);
                    const @T@ cut   = d_internal_math_@S@_abs(diff) / half;
                    const bool   far   = (cut >= d_internal_math_@S@_abs(twoa));
                    const @T@ theta = far ? (@T@)0 : (diff / twoa);
                    @T@       t     = (@T@)1;
                    @T@       c     = (@T@)0;
                    @T@       s     = (@T@)0;

                    // the smaller root of t^2 + 2 theta t - 1 = 0; past
                    // 2^(EMAX / 2), where theta^2 would overflow, 1 / (2
                    // theta), apq / diff, which it equals within a part in
                    // 2^EMAX
                    if ( (far) ||
                         (d_internal_math_@S@_abs(theta) >= half) )
                    {
                        t = apq / diff;
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

                    // the columns p and q, then the rows, then V's columns
                    for (i = 0u; i < _n; ++i)
                    {
                        const @T@ wip = _work[(i * _n) + p];
                        const @T@ wiq = _work[(i * _n) + q];
                        const @T@ a1  = c * wip;
                        const @T@ a2  = s * wiq;
                        const @T@ b1  = s * wip;
                        const @T@ b2  = c * wiq;

                        _work[(i * _n) + p] = a1 - a2;
                        _work[(i * _n) + q] = b1 + b2;
                    }

                    for (i = 0u; i < _n; ++i)
                    {
                        const @T@ wpj = _work[(p * _n) + i];
                        const @T@ wqj = _work[(q * _n) + i];
                        const @T@ a1  = c * wpj;
                        const @T@ a2  = s * wqj;
                        const @T@ b1  = s * wpj;
                        const @T@ b2  = c * wqj;

                        _work[(p * _n) + i] = a1 - a2;
                        _work[(q * _n) + i] = b1 + b2;
                    }

                    for (i = 0u; i < _n; ++i)
                    {
                        const @T@ vip = _vectors[(i * _n) + p];
                        const @T@ viq = _vectors[(i * _n) + q];
                        const @T@ a1  = c * vip;
                        const @T@ a2  = s * viq;
                        const @T@ b1  = s * vip;
                        const @T@ b2  = c * viq;

                        _vectors[(i * _n) + p] = a1 - a2;
                        _vectors[(i * _n) + q] = b1 + b2;
                    }
                }
            }
        }
    }

    for (i = 0u; i < _n; ++i)
    {
        _values[i] = _work[(i * _n) + i];
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
                const @T@ t = _vectors[(k * _n) + i];

                _vectors[(k * _n) + i]   = _vectors[(k * _n) + sel];
                _vectors[(k * _n) + sel] = t;
            }
        }
    }

    return;
}

// d_decomp_@S@_cholesky@F@
//   the Cholesky factor of the symmetric n-by-n _a (its lower triangle
// read) into _l, zero on entry: A = L L^T. Returns whether A is positive
// definite; at the first non-positive pivot it stops, _l filled to there.
D_MATH_FN@RT@ bool
d_decomp_@S@_cholesky@F@(
    @T@*       _l,
    const @T@* _a,
    size_t     _n
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j <= i; ++j)
        {
            @T@    sum = _a[(i * _n) + j];
            size_t k   = 0u;

            for (k = 0u; k < j; ++k)
            {
                const @T@ prod = _l[(i * _n) + k] * _l[(j * _n) + k];

                sum = sum - prod;
            }

            if (i == j)
            {
                // not positive definite: stop
                if (sum <= (@T@)0)
                {
                    return false;
                }

                _l[(i * _n) + j] = d_math_@S@_sqrt@F@(sum);
            }
            else
            {
                _l[(i * _n) + j] = sum / _l[(j * _n) + j];
            }
        }
    }

    return true;
}
"""
