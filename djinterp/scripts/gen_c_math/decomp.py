"""gen_c_math.decomp - the text of decomp.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (DATE, FAMILIES, banner, instantiate, overload_section)
from .decomp_text import (DECOMP_FORMS, DECOMP_TEMPLATE)

DECOMP_EIGEN_FORMS = r"""
// d_internal_decomp_@S@_rotation_of@F@
//   the plane rotation that takes (_x, _y) to (r, 0): c = _x / r and
// s = _y / r, formed on the pair divided by its larger magnitude, so that
// the sum of squares neither overflows nor underflows. (1, 0, 0) for
// (0, 0).
D_MATH_FN@RT@ struct dj_internal_decomp_@S@_rotation
d_internal_decomp_@S@_rotation_of@F@(
    @T@ _x,
    @T@ _y
)
{
    struct dj_internal_decomp_@S@_rotation g =
        { (@T@)1, (@T@)0, (@T@)0 };
    const @T@ ax = d_internal_math_@S@_abs(_x);
    const @T@ ay = d_internal_math_@S@_abs(_y);
    const @T@ m  = (ax > ay) ? ax : ay;

    // nothing to rotate
    if (m == (@T@)0)
    {
        return g;
    }

    {
        const @T@ xs   = _x / m;
        const @T@ ys   = _y / m;
        const @T@ xx   = xs * xs;
        const @T@ yy   = ys * ys;
        const @T@ sum  = xx + yy;
        const @T@ root = d_math_@S@_sqrt@F@(sum);

        g.c = xs / root;
        g.s = ys / root;
        g.r = m * root;
    }

    return g;
}

// d_decomp_@S@_hessenberg@F@
//   reduces the n-by-n _h in place to upper Hessenberg form by plane
// rotations: column by column, each entry below the subdiagonal from the
// bottom up is rotated into the one above it, the rotation applied to both
// rows and both columns - a similarity, so the eigenvalues stay. The
// entries below the subdiagonal end exactly zero. Q is not formed.
D_MATH_FN@RT@ void
d_decomp_@S@_hessenberg@F@(
    @T@*   _h,
    size_t _n
)
{
    size_t j = 0u;

    for (j = 0u; (j + 2u) < _n; ++j)
    {
        size_t i = 0u;

        // the column's entries below the subdiagonal, the last row's first
        for (i = _n - 1u; i >= (j + 2u); --i)
        {
            const @T@ below = _h[(i * _n) + j];

            // already zero: nothing to rotate
            if (below == (@T@)0)
            {
                continue;
            }

            {
                const @T@ above = _h[((i - 1u) * _n) + j];
                const struct dj_internal_decomp_@S@_rotation g =
                    d_internal_decomp_@S@_rotation_of@F@(above, below);
                size_t k = 0u;

                // rows i - 1 and i, from column j on
                for (k = j; k < _n; ++k)
                {
                    const @T@ t1 = _h[((i - 1u) * _n) + k];
                    const @T@ t2 = _h[(i * _n) + k];
                    const @T@ a1 = g.c * t1;
                    const @T@ a2 = g.s * t2;
                    const @T@ b1 = g.c * t2;
                    const @T@ b2 = g.s * t1;

                    _h[((i - 1u) * _n) + k] = a1 + a2;
                    _h[(i * _n) + k]        = b1 - b2;
                }

                // the rotated column, exactly (r, 0)
                _h[((i - 1u) * _n) + j] = g.r;
                _h[(i * _n) + j]        = (@T@)0;

                // columns i - 1 and i, every row
                for (k = 0u; k < _n; ++k)
                {
                    const @T@ t1 = _h[(k * _n) + (i - 1u)];
                    const @T@ t2 = _h[(k * _n) + i];
                    const @T@ a1 = g.c * t1;
                    const @T@ a2 = g.s * t2;
                    const @T@ b1 = g.c * t2;
                    const @T@ b2 = g.s * t1;

                    _h[(k * _n) + (i - 1u)] = a1 + a2;
                    _h[(k * _n) + i]        = b1 - b2;
                }
            }
        }
    }

    return;
}

// d_internal_decomp_@S@_hypot@F@
//   sqrt(_x^2 + _y^2), as LAPACK's dlapy2: the larger magnitude times the
// root of 1 and the smaller's ratio to it squared, so that nothing on the
// way overflows or underflows; the larger magnitude itself where the
// smaller is zero or the larger infinite; a NaN for a NaN.
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_hypot@F@(
    @T@ _x,
    @T@ _y
)
{
    const @T@ ax = d_internal_math_@S@_abs(_x);
    const @T@ ay = d_internal_math_@S@_abs(_y);
    const @T@ w  = (ax > ay) ? ax : ay;
    const @T@ z  = (ax > ay) ? ay : ax;

    // a NaN
    if ( (_x != _x) ||
         (_y != _y) )
    {
        return d_internal_math_@S@_nan();
    }

    // nothing to add, or an infinity
    if ( (z == (@T@)0) ||
         (w > @MAX@) )
    {
        return w;
    }

    {
        const @T@ ratio = z / w;
        const @T@ sq    = ratio * ratio;
        const @T@ one   = (@T@)1 + sq;
        const @T@ root  = d_math_@S@_sqrt@F@(one);

        return w * root;
    }
}

// d_internal_decomp_@S@_norm@F@
//   the Euclidean norm of the _count entries _x[0], _x[_stride], ...: the
// root of their sum of squares; where that norm passes the family's range or
// falls below the root of its least normal over its epsilon, the norm of the
// entries scaled exactly by a power of two to a largest magnitude in [1, 2),
// scaled back (S256); with a NaN or an infinity among them, or all zero, the
// sum's own root.
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_norm@F@(
    const @T@* _x,
    size_t     _count,
    size_t     _stride
)
{
    const @T@ small   = d_math_@S@_sqrt@F@(@MIN@ / @EPS@);
    @T@       sum     = (@T@)0;
    @T@       norm    = (@T@)0;
    @T@       largest = (@T@)0;
    size_t    i       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ v  = _x[i * _stride];
        const @T@ sq = v * v;

        sum = sum + sq;
    }

    norm = d_math_@S@_sqrt@F@(sum);

    // in range: the sum's root
    if ( (norm <= @MAX@) &&
         (!(norm < small)) )
    {
        return norm;
    }

    // the largest magnitude; a NaN or an infinity, the sum's root
    for (i = 0u; i < _count; ++i)
    {
        const @T@ v = d_internal_math_@S@_abs(_x[i * _stride]);

        if (!(v <= @MAX@))
        {
            return norm;
        }

        if (v > largest)
        {
            largest = v;
        }
    }

    // the zero vector
    if (largest == (@T@)0)
    {
        return norm;
    }

    {
        @T@ pre   = (@T@)1;
        @T@ power = (@T@)1;

        // a subnormal largest: lifted into the normal range first
        if (largest < @MIN@)
        {
            pre     = d_internal_math_@S@_pow2(@P@);
            largest = largest * pre;
        }

        {
            const struct dj_internal_math_@S@_binade b =
                d_internal_math_@S@_binade_of(largest);

            power = b.power;
        }

        sum = (@T@)0;

        for (i = 0u; i < _count; ++i)
        {
            const @T@ lifted = _x[i * _stride] * pre;
            const @T@ v      = lifted / power;
            const @T@ sq     = v * v;

            sum = sum + sq;
        }

        {
            const @T@ root     = d_math_@S@_sqrt@F@(sum);
            const @T@ restored = root * power;

            return restored / pre;
        }
    }
}

// d_decomp_@S@_balance@F@
//   balances the n-by-n _a in place, as LAPACK's dgebal does with job 'B':
// the rows and columns that isolate an eigenvalue permuted to the ends -
// each row zero off the diagonal, within the columns not yet isolated, to
// the bottom, then each such column to the top - so that the eigenvalues
// outside rows *_ilo to *_ihi stand on the diagonal; then each row and
// column inside, by turns, scaled by a power of two, its row divided and its
// column multiplied, wherever that brings the sum of the two norms below
// 0.95 of what it was, until none does: a diagonal similarity, exact unless
// an entry passes below the least normal on the way, as dgebal lets it
// (S311). The eigenvalues stay; the norm, before a QR iteration, falls.
// _scale (n entries) receives each factor, 1 outside *_ilo..*_ihi; _perm
// (n entries), unless NULL, each outside row's swap. Indices count from 0.
D_MATH_FN_KERNEL@RT@ void
d_decomp_@S@_balance@F@(
    @T@*    _a,
    size_t  _n,
    size_t* _perm,
    @T@*    _scale,
    size_t* _ilo,
    size_t* _ihi
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
                     (_a[(i * _n) + j] != (@T@)0) )
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
                    d_internal_decomp_@S@_swap_isolated(_a, _n, i, l, k, l);
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
                     (_a[(i * _n) + j] != (@T@)0) )
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
                    d_internal_decomp_@S@_swap_isolated(_a, _n, j, k, k, l);
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
            @T@          c     = d_internal_decomp_@S@_norm@F@(
                                     _a + ((k * _n) + i), count, _n);
            @T@          r     = d_internal_decomp_@S@_norm@F@(
                                     _a + ((i * _n) + k), count, 1u);
            @T@          ca    = (@T@)0;
            @T@          ra    = (@T@)0;
            @T@          g     = (@T@)0;
            @T@          f     = (@T@)1;
            @T@          s     = (@T@)0;

            for (j = 0u; j <= l; ++j)
            {
                const @T@ v = d_internal_math_@S@_abs(_a[(j * _n) + i]);

                if (v > ca)
                {
                    ca = v;
                }
            }

            for (j = k; j < _n; ++j)
            {
                const @T@ v = d_internal_math_@S@_abs(_a[(i * _n) + j]);

                if (v > ra)
                {
                    ra = v;
                }
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
                _a[(i * _n) + j] = _a[(i * _n) + j] * g;
            }

            for (j = 0u; j <= l; ++j)
            {
                _a[(j * _n) + i] = _a[(j * _n) + i] * f;
            }
        }
    }

    *_ilo = k;
    *_ihi = l;

    return;
}

// d_internal_decomp_@S@_reflector@F@
//   the elementary reflector H = I - tau v v^T, v = (1, x), x the _count
// entries _x[0], _x[_stride], ..., that takes (*_alpha, x) to (beta, 0), as
// LAPACK's dlarfg: beta = -sign(alpha) times the pair's norm (alpha's
// sign, +, for a zero), tau = (beta - alpha) / beta, x multiplied by
// 1 / (alpha - beta); where beta is below the family's least normal over
// its epsilon, the vector is lifted first and beta lowered after. *_alpha
// becomes beta, _x becomes v's tail, and tau is returned: 0, H = I, where x
// is zero.
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_reflector@F@(
    @T@*   _alpha,
    @T@*   _x,
    size_t _count,
    size_t _stride
)
{
    const @T@ safmin = @MIN@ / (@EPS@ / (@T@)2);
    const @T@ rsafmn = (@T@)1 / safmin;
    @T@       xnorm  = d_internal_decomp_@S@_norm@F@(_x, _count, _stride);
    @T@       beta   = (@T@)0;
    @T@       tau    = (@T@)0;
    size_t    knt    = 0u;
    size_t    i      = 0u;

    // x zero: H = I
    if (xnorm == (@T@)0)
    {
        return (@T@)0;
    }

    {
        const @T@ h = d_internal_decomp_@S@_hypot@F@(*_alpha, xnorm);

        beta = (*_alpha < (@T@)0) ? h : -h;
    }

    // beta below the safe minimum: lifted, and found again
    if (d_internal_math_@S@_abs(beta) < safmin)
    {
        do
        {
            ++knt;

            for (i = 0u; i < _count; ++i)
            {
                _x[i * _stride] = _x[i * _stride] * rsafmn;
            }

            beta    = beta * rsafmn;
            *_alpha = *_alpha * rsafmn;
        }
        while ( (d_internal_math_@S@_abs(beta) < safmin) &&
                (knt < 20u) );

        xnorm = d_internal_decomp_@S@_norm@F@(_x, _count, _stride);

        {
            const @T@ h = d_internal_decomp_@S@_hypot@F@(*_alpha, xnorm);

            beta = (*_alpha < (@T@)0) ? h : -h;
        }
    }

    {
        const @T@ diff = beta - *_alpha;
        const @T@ den  = *_alpha - beta;
        const @T@ rec  = (@T@)1 / den;

        tau = diff / beta;

        for (i = 0u; i < _count; ++i)
        {
            _x[i * _stride] = _x[i * _stride] * rec;
        }
    }

    // lowered back
    for (i = 0u; i < knt; ++i)
    {
        beta = beta * safmin;
    }

    *_alpha = beta;

    return tau;
}

// d_decomp_@S@_hessenberg_q@F@
//   reduces rows and columns _ilo to _ihi of the n-by-n _h to upper
// Hessenberg form in place by Householder reflectors, as LAPACK's dgehd2:
// for each column from _ilo, the reflector that zeroes it below the
// subdiagonal, applied from the right to rows 0 to _ihi and from the left to
// columns to the end - a similarity, H = Q^T A Q, so the eigenvalues stay.
// The rows and columns outside, as balancing leaves them, keep their zeros.
// _q, unless NULL, the n-by-n identity on entry, receives Q, each reflector
// multiplied in on the right as it is made. The entries below the
// subdiagonal end exactly zero.
D_MATH_FN_KERNEL@RT@ void
d_decomp_@S@_hessenberg_q@F@(
    @T@*   _h,
    size_t _n,
    size_t _ilo,
    size_t _ihi,
    @T@*   _q
)
{
    size_t i = 0u;

    // nothing to reduce
    if ( (_n == 0u) ||
         (_ihi <= _ilo) )
    {
        return;
    }

    for (i = _ilo; (i + 1u) < _ihi; ++i)
    {
        const size_t below = _ihi - i - 1u;
        @T@          alpha = _h[((i + 1u) * _n) + i];
        const @T@    tau   = d_internal_decomp_@S@_reflector@F@(
                                 &alpha, _h + (((i + 2u) * _n) + i), below,
                                 _n);
        size_t       r     = 0u;
        size_t       c     = 0u;

        _h[((i + 1u) * _n) + i] = alpha;

        // H = I: nothing to apply
        if (tau != (@T@)0)
        {
            // from the right: rows 0 to ihi, columns i + 1 to ihi
            for (r = 0u; r <= _ihi; ++r)
            {
                @T@ w = _h[(r * _n) + (i + 1u)];

                for (c = i + 2u; c <= _ihi; ++c)
                {
                    const @T@ p = _h[(r * _n) + c] * _h[(c * _n) + i];

                    w = w + p;
                }

                {
                    const @T@ t = tau * w;

                    _h[(r * _n) + (i + 1u)] = _h[(r * _n) + (i + 1u)] - t;

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        const @T@ p = t * _h[(c * _n) + i];

                        _h[(r * _n) + c] = _h[(r * _n) + c] - p;
                    }
                }
            }

            // from the left: rows i + 1 to ihi, columns i + 1 to the end
            for (c = i + 1u; c < _n; ++c)
            {
                @T@ w = _h[((i + 1u) * _n) + c];

                for (r = i + 2u; r <= _ihi; ++r)
                {
                    const @T@ p = _h[(r * _n) + i] * _h[(r * _n) + c];

                    w = w + p;
                }

                {
                    const @T@ t = tau * w;

                    _h[((i + 1u) * _n) + c] = _h[((i + 1u) * _n) + c] - t;

                    for (r = i + 2u; r <= _ihi; ++r)
                    {
                        const @T@ p = _h[(r * _n) + i] * t;

                        _h[(r * _n) + c] = _h[(r * _n) + c] - p;
                    }
                }
            }

            // Q's columns i + 1 to ihi, from the right
            if (_q != NULL)
            {
                for (r = _ilo; r <= _ihi; ++r)
                {
                    @T@ w = _q[(r * _n) + (i + 1u)];

                    for (c = i + 2u; c <= _ihi; ++c)
                    {
                        const @T@ p = _q[(r * _n) + c] * _h[(c * _n) + i];

                        w = w + p;
                    }

                    {
                        const @T@ t = tau * w;

                        _q[(r * _n) + (i + 1u)] =
                            _q[(r * _n) + (i + 1u)] - t;

                        for (c = i + 2u; c <= _ihi; ++c)
                        {
                            const @T@ p = t * _h[(c * _n) + i];

                            _q[(r * _n) + c] = _q[(r * _n) + c] - p;
                        }
                    }
                }
            }
        }

        // the reflector's vector, kept below the subdiagonal until now
        for (r = i + 2u; r <= _ihi; ++r)
        {
            _h[(r * _n) + i] = (@T@)0;
        }
    }

    return;
}

// d_internal_decomp_@S@_standard_pair@F@
//   the 2-by-2 block [[*_a, *_b], [*_c, *_d]] brought to its standard Schur
// form by a rotation [[cs, -sn], [sn, cs]], as LAPACK's dlanv2: upper
// triangular where its eigenvalues are real, else with equal diagonals and
// off-diagonals of opposite signs. Its eigenvalues go to _values[0] and
// _values[1]: a real one's imaginary part exactly +0, a complex pair's
// sqrt(|b|) sqrt(|c|) and its negation, so that the two are exactly
// conjugate, the positive first.
D_MATH_FN@RT@ void
d_internal_decomp_@S@_standard_pair@F@(
    @T@*                _a,
    @T@*                _b,
    @T@*                _c,
    @T@*                _d,
    struct dj_cplx_@S@* _values,
    @T@*                _cs,
    @T@*                _sn
)
{
    const @T@ eps    = @EPS@;
    const @T@ safmn2 = d_internal_math_@S@_pow2((@EMIN@ - 2 + @P@) / 2);
    const @T@ safmx2 = (@T@)1 / safmn2;
    @T@       a      = *_a;
    @T@       b      = *_b;
    @T@       c      = *_c;
    @T@       d      = *_d;
    @T@       cs     = (@T@)1;
    @T@       sn     = (@T@)0;

    // already triangular
    if (c == (@T@)0)
    {
        cs = (@T@)1;
        sn = (@T@)0;
    }
    // lower triangular: the rows and columns swapped
    else if (b == (@T@)0)
    {
        const @T@ temp = d;

        cs = (@T@)0;
        sn = (@T@)1;
        d  = a;
        a  = temp;
        b  = -c;
        c  = (@T@)0;
    }
    // already standard: equal diagonals, off-diagonals of opposite signs
    else if ( ((a - d) == (@T@)0) &&
              ((b < (@T@)0) != (c < (@T@)0)) )
    {
        cs = (@T@)1;
        sn = (@T@)0;
    }
    else
    {
        const @T@ temp0 = a - d;
        @T@       p     = temp0 / (@T@)2;
        const @T@ ab    = d_internal_math_@S@_abs(b);
        const @T@ ac    = d_internal_math_@S@_abs(c);
        const @T@ bcmax = (ab > ac) ? ab : ac;
        const @T@ bcmn  = (ab > ac) ? ac : ab;
        const @T@ sgnb  = (b < (@T@)0) ? (@T@)-1 : (@T@)1;
        const @T@ sgnc  = (c < (@T@)0) ? (@T@)-1 : (@T@)1;
        const @T@ bcmis = (bcmn * sgnb) * sgnc;
        const @T@ ap    = d_internal_math_@S@_abs(p);
        const @T@ scale = (ap > bcmax) ? ap : bcmax;
        @T@       z     = (@T@)0;

        {
            const @T@ r1 = p / scale;
            const @T@ t1 = r1 * p;
            const @T@ r2 = bcmax / scale;
            const @T@ t2 = r2 * bcmis;

            z = t1 + t2;
        }

        // real eigenvalues, clearly: A and D, and the rotation
        if (z >= ((@T@)4 * eps))
        {
            const @T@ rs   = d_math_@S@_sqrt@F@(scale);
            const @T@ rz   = d_math_@S@_sqrt@F@(z);
            const @T@ root = rs * rz;
            @T@       tau  = (@T@)0;

            z = (p < (@T@)0) ? (p - root) : (p + root);
            a = d + z;

            {
                const @T@ q = bcmax / z;
                const @T@ m = q * bcmis;

                d = d - m;
            }

            tau = d_internal_decomp_@S@_hypot@F@(c, z);
            cs  = z / tau;
            sn  = c / tau;
            b   = b - c;
            c   = (@T@)0;
        }
        // complex, or real and nearly equal: the diagonals made equal
        else
        {
            @T@    sigma = b + c;
            @T@    temp  = temp0;
            size_t count = 0u;
            @T@    tau   = (@T@)0;

            // sigma and temp brought into the safe range
            for (;;)
            {
                const @T@ at  = d_internal_math_@S@_abs(temp);
                const @T@ asg = d_internal_math_@S@_abs(sigma);
                const @T@ big = (at > asg) ? at : asg;

                ++count;

                if (big >= safmx2)
                {
                    sigma = sigma * safmn2;
                    temp  = temp * safmn2;

                    if (count <= 20u)
                    {
                        continue;
                    }
                }

                if (big <= safmn2)
                {
                    sigma = sigma * safmx2;
                    temp  = temp * safmx2;

                    if (count <= 20u)
                    {
                        continue;
                    }
                }

                break;
            }

            p   = temp / (@T@)2;
            tau = d_internal_decomp_@S@_hypot@F@(sigma, temp);

            {
                const @T@ ratio = d_internal_math_@S@_abs(sigma) / tau;
                const @T@ one   = (@T@)1 + ratio;
                const @T@ half  = one / (@T@)2;

                cs = d_math_@S@_sqrt@F@(half);

                {
                    const @T@ den = tau * cs;
                    const @T@ q   = p / den;

                    sn = (sigma < (@T@)0) ? q : -q;
                }
            }

            {
                // [aa bb; cc dd] = [a b; c d] [cs -sn; sn cs]
                const @T@ a1 = a * cs;
                const @T@ a2 = b * sn;
                const @T@ b1 = a * sn;
                const @T@ b2 = b * cs;
                const @T@ c1 = c * cs;
                const @T@ c2 = d * sn;
                const @T@ d1 = c * sn;
                const @T@ d2 = d * cs;
                const @T@ aa = a1 + a2;
                const @T@ bb = b2 - b1;
                const @T@ cc = c1 + c2;
                const @T@ dd = d2 - d1;

                // [a b; c d] = [cs sn; -sn cs] [aa bb; cc dd]
                {
                    const @T@ e1 = aa * cs;
                    const @T@ e2 = cc * sn;
                    const @T@ f1 = bb * cs;
                    const @T@ f2 = dd * sn;
                    const @T@ g1 = aa * sn;
                    const @T@ g2 = cc * cs;
                    const @T@ h1 = bb * sn;
                    const @T@ h2 = dd * cs;

                    a = e1 + e2;
                    b = f1 + f2;
                    c = g2 - g1;
                    d = h2 - h1;
                }
            }

            {
                const @T@ sum = a + d;

                temp = sum / (@T@)2;
            }

            a = temp;
            d = temp;

            if (c != (@T@)0)
            {
                if (b != (@T@)0)
                {
                    // real after all: reduced to upper triangular
                    if ((b < (@T@)0) == (c < (@T@)0))
                    {
                        const @T@ sab  = d_math_@S@_sqrt@F@(
                                             d_internal_math_@S@_abs(b));
                        const @T@ sac  = d_math_@S@_sqrt@F@(
                                             d_internal_math_@S@_abs(c));
                        const @T@ prod = sab * sac;
                        const @T@ pp   = (c < (@T@)0) ? -prod : prod;
                        const @T@ bc   = b + c;
                        const @T@ rt   = d_math_@S@_sqrt@F@(
                                             d_internal_math_@S@_abs(bc));
                        const @T@ t2   = (@T@)1 / rt;
                        const @T@ cs1  = sab * t2;
                        const @T@ sn1  = sac * t2;

                        a = temp + pp;
                        d = temp - pp;
                        b = b - c;
                        c = (@T@)0;

                        {
                            const @T@ m1 = cs * cs1;
                            const @T@ m2 = sn * sn1;
                            const @T@ m3 = cs * sn1;
                            const @T@ m4 = sn * cs1;
                            const @T@ nc = m1 - m2;

                            sn = m3 + m4;
                            cs = nc;
                        }
                    }
                }
                else
                {
                    const @T@ t = cs;

                    b  = -c;
                    c  = (@T@)0;
                    cs = -sn;
                    sn = t;
                }
            }
        }
    }

    *_a  = a;
    *_b  = b;
    *_c  = c;
    *_d  = d;
    *_cs = cs;
    *_sn = sn;

    _values[0].part[0] = a;
    _values[1].part[0] = d;

    // real
    if (c == (@T@)0)
    {
        _values[0].part[1] = (@T@)0;
        _values[1].part[1] = (@T@)0;
    }
    // a complex pair, exactly conjugate
    else
    {
        const @T@ rb = d_math_@S@_sqrt@F@(d_internal_math_@S@_abs(b));
        const @T@ rc = d_math_@S@_sqrt@F@(d_internal_math_@S@_abs(c));
        const @T@ im = rb * rc;

        _values[0].part[1] = im;
        _values[1].part[1] = -im;
    }

    return;
}

// d_internal_decomp_@S@_schur@F@
//   d_decomp_@S@_schur@F@'s iteration, its eigenvalues put by
// d_internal_decomp_@S@_put: into _values, unless NULL, and their real
// parts into _real, unless NULL, *_complex, unless NULL, set where one is
// not real.
D_MATH_FN_KERNEL@RT@ bool
d_internal_decomp_@S@_schur@F@(
    @T@*                _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    @T@*                _z,
    struct dj_cplx_@S@* _values,
    @T@*                _real,
    bool*               _complex,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const @T@    dat1   = (@T@)0.75;
    const @T@    dat2   = (@T@)-0.4375;
    const size_t kexsh  = 10u;
    const @T@    ulp    = @EPS@;
    const @T@    tol    = (_tol > ulp) ? _tol : ulp;
    const bool   wantt  = (_z != NULL);
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
            d_internal_decomp_@S@_put(_values, _real, _complex, j,
                                      _h[(j * _n) + j], (@T@)0);
        }
    }

    // a block of one
    if (_ilo == _ihi)
    {
        d_internal_decomp_@S@_put(_values, _real, _complex, _ilo,
                                  _h[(_ilo * _n) + _ilo], (@T@)0);

        return true;
    }

    // what lies below the subdiagonal, cleared
    for (j = _ilo; (j + 3u) <= _ihi; ++j)
    {
        _h[((j + 2u) * _n) + j] = (@T@)0;
        _h[((j + 3u) * _n) + j] = (@T@)0;
    }

    if ((_ilo + 2u) <= _ihi)
    {
        _h[(_ihi * _n) + (_ihi - 2u)] = (@T@)0;
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
            size_t m = 0u;
            @T@    v[3] = { (@T@)0, (@T@)0, (@T@)0 };
            @T@    rt1r = (@T@)0;
            @T@    rt1i = (@T@)0;
            @T@    rt2r = (@T@)0;
            @T@    rt2i = (@T@)0;

            // the lowest negligible subdiagonal entry
            for (k = i; k > l; --k)
            {
                const @T@ sub = d_internal_math_@S@_abs(
                                    _h[(k * _n) + (k - 1u)]);
                @T@       tst = (@T@)0;

                if (sub <= smlnum)
                {
                    break;
                }

                {
                    const @T@ d1 = d_internal_math_@S@_abs(
                                       _h[((k - 1u) * _n) + (k - 1u)]);
                    const @T@ d2 = d_internal_math_@S@_abs(
                                       _h[(k * _n) + k]);

                    tst = d1 + d2;
                }

                // both neighbours zero: the next ones out
                if (tst == (@T@)0)
                {
                    if (k >= (_ilo + 2u))
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k - 1u) * _n) + (k - 2u)]);
                    }

                    if ((k + 1u) <= _ihi)
                    {
                        tst = tst + d_internal_math_@S@_abs(
                                        _h[((k + 1u) * _n) + k]);
                    }
                }

                {
                    const @T@ bound = tol * tst;

                    // Ahues and Kressner's test
                    if (sub <= bound)
                    {
                        const @T@ sup  = d_internal_math_@S@_abs(
                                             _h[((k - 1u) * _n) + k]);
                        const @T@ ab   = (sub > sup) ? sub : sup;
                        const @T@ ba   = (sub > sup) ? sup : sub;
                        const @T@ hkk  = d_internal_math_@S@_abs(
                                             _h[(k * _n) + k]);
                        const @T@ diff = _h[((k - 1u) * _n) + (k - 1u)] -
                                         _h[(k * _n) + k];
                        const @T@ ad   = d_internal_math_@S@_abs(diff);
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
                _h[(l * _n) + (l - 1u)] = (@T@)0;
            }

            // a block of one or two split off
            if ((l + 1u) >= i)
            {
                break;
            }

            // the budget spent: the rows not reached, their diagonal
            if (steps >= _max_iter)
            {
                for (j = _ilo; j <= i; ++j)
                {
                    d_internal_decomp_@S@_put(_values, _real, _complex, j,
                                              _h[(j * _n) + j], (@T@)0);
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

            {
                @T@ h11 = (@T@)0;
                @T@ h12 = (@T@)0;
                @T@ h21 = (@T@)0;
                @T@ h22 = (@T@)0;
                @T@ s   = (@T@)0;

                // exceptional shifts, at the bottom, then at the top
                if ((kdefl % (2u * kexsh)) == 0u)
                {
                    const @T@ e1 = d_internal_math_@S@_abs(
                                       _h[(i * _n) + (i - 1u)]);
                    const @T@ e2 = d_internal_math_@S@_abs(
                                       _h[((i - 1u) * _n) + (i - 2u)]);

                    s = e1 + e2;

                    {
                        const @T@ p = dat1 * s;

                        h11 = p + _h[(i * _n) + i];
                    }

                    h12 = dat2 * s;
                    h21 = s;
                    h22 = h11;
                }
                else if ((kdefl % kexsh) == 0u)
                {
                    const @T@ e1 = d_internal_math_@S@_abs(
                                       _h[((l + 1u) * _n) + l]);
                    const @T@ e2 = d_internal_math_@S@_abs(
                                       _h[((l + 2u) * _n) + (l + 1u)]);

                    s = e1 + e2;

                    {
                        const @T@ p = dat1 * s;

                        h11 = p + _h[(l * _n) + l];
                    }

                    h12 = dat2 * s;
                    h21 = s;
                    h22 = h11;
                }
                // Francis's: the trailing 2-by-2
                else
                {
                    h11 = _h[((i - 1u) * _n) + (i - 1u)];
                    h21 = _h[(i * _n) + (i - 1u)];
                    h12 = _h[((i - 1u) * _n) + i];
                    h22 = _h[(i * _n) + i];
                }

                {
                    const @T@ a1 = d_internal_math_@S@_abs(h11);
                    const @T@ a2 = d_internal_math_@S@_abs(h12);
                    const @T@ a3 = d_internal_math_@S@_abs(h21);
                    const @T@ a4 = d_internal_math_@S@_abs(h22);
                    const @T@ s1 = a1 + a2;
                    const @T@ s2 = s1 + a3;

                    s = s2 + a4;
                }

                // the shifts: the block's eigenvalues, scaled
                if (s != (@T@)0)
                {
                    @T@ tr     = (@T@)0;
                    @T@ det    = (@T@)0;
                    @T@ rtdisc = (@T@)0;

                    h11 = h11 / s;
                    h21 = h21 / s;
                    h12 = h12 / s;
                    h22 = h22 / s;

                    {
                        const @T@ sum = h11 + h22;

                        tr = sum / (@T@)2;
                    }

                    {
                        const @T@ e1 = h11 - tr;
                        const @T@ e2 = h22 - tr;
                        const @T@ p1 = e1 * e2;
                        const @T@ p2 = h12 * h21;

                        det = p1 - p2;
                    }

                    rtdisc = d_math_@S@_sqrt@F@(
                                 d_internal_math_@S@_abs(det));

                    // complex conjugate shifts
                    if (det >= (@T@)0)
                    {
                        rt1r = tr * s;
                        rt2r = rt1r;
                        rt1i = rtdisc * s;
                        rt2i = -rt1i;
                    }
                    // real shifts: the one nearer h22, twice
                    else
                    {
                        const @T@ x1 = tr + rtdisc;
                        const @T@ x2 = tr - rtdisc;
                        const @T@ d1 = x1 - h22;
                        const @T@ d2 = x2 - h22;

                        if (d_internal_math_@S@_abs(d1) <=
                            d_internal_math_@S@_abs(d2))
                        {
                            rt1r = x1 * s;
                            rt2r = rt1r;
                        }
                        else
                        {
                            rt2r = x2 * s;
                            rt1r = rt2r;
                        }

                        rt1i = (@T@)0;
                        rt2i = (@T@)0;
                    }
                }
            }

            // the row the step starts at: two small subdiagonal entries
            for (m = i - 2u; ; --m)
            {
                const @T@ hmm  = _h[(m * _n) + m];
                const @T@ hm1  = _h[((m + 1u) * _n) + (m + 1u)];
                @T@       h21s = _h[((m + 1u) * _n) + m];
                @T@       s    = (@T@)0;

                {
                    const @T@ e  = hmm - rt2r;
                    const @T@ a1 = d_internal_math_@S@_abs(e);
                    const @T@ a2 = d_internal_math_@S@_abs(rt2i);
                    const @T@ a3 = d_internal_math_@S@_abs(h21s);
                    const @T@ s1 = a1 + a2;

                    s = s1 + a3;
                }

                h21s = _h[((m + 1u) * _n) + m] / s;

                {
                    const @T@ p1 = h21s * _h[(m * _n) + (m + 1u)];
                    const @T@ e1 = hmm - rt1r;
                    const @T@ e2 = hmm - rt2r;
                    const @T@ q2 = e2 / s;
                    const @T@ p2 = e1 * q2;
                    const @T@ q3 = rt2i / s;
                    const @T@ p3 = rt1i * q3;
                    const @T@ s1 = p1 + p2;

                    v[0] = s1 - p3;
                }

                {
                    const @T@ s1 = hmm + hm1;
                    const @T@ s2 = s1 - rt1r;
                    const @T@ s3 = s2 - rt2r;

                    v[1] = h21s * s3;
                }

                v[2] = h21s * _h[((m + 2u) * _n) + (m + 1u)];

                {
                    const @T@ a1 = d_internal_math_@S@_abs(v[0]);
                    const @T@ a2 = d_internal_math_@S@_abs(v[1]);
                    const @T@ a3 = d_internal_math_@S@_abs(v[2]);
                    const @T@ s1 = a1 + a2;

                    s = s1 + a3;
                }

                v[0] = v[0] / s;
                v[1] = v[1] / s;
                v[2] = v[2] / s;

                // the top of the block
                if (m == l)
                {
                    break;
                }

                {
                    const @T@ a0  = d_internal_math_@S@_abs(v[0]);
                    const @T@ a1  = d_internal_math_@S@_abs(v[1]);
                    const @T@ a2  = d_internal_math_@S@_abs(v[2]);
                    const @T@ e1  = d_internal_math_@S@_abs(
                                        _h[(m * _n) + (m - 1u)]);
                    const @T@ e2  = d_internal_math_@S@_abs(
                                        _h[((m - 1u) * _n) + (m - 1u)]);
                    const @T@ e3  = d_internal_math_@S@_abs(hmm);
                    const @T@ e4  = d_internal_math_@S@_abs(hm1);
                    const @T@ s12 = a1 + a2;
                    const @T@ h00 = e1 * s12;
                    const @T@ f1  = ulp * a0;
                    const @T@ g1  = e2 + e3;
                    const @T@ g2  = g1 + e4;
                    const @T@ h01 = f1 * g2;

                    // starting here leaves H(m, m - 1) negligible
                    if (h00 <= h01)
                    {
                        break;
                    }
                }
            }

            // the double-shift step: the bulge made, then chased out
            for (k = m; (k + 1u) <= i; ++k)
            {
                const size_t nr  = ((i - k) + 1u < 3u) ? ((i - k) + 1u) : 3u;
                const bool   three = (nr == 3u);
                @T@          t1  = (@T@)0;
                @T@          v2  = (@T@)0;
                @T@          v3  = (@T@)0;
                @T@          t2  = (@T@)0;
                @T@          t3  = (@T@)0;
                size_t       jj  = 0u;
                size_t       last = 0u;

                // past the first: the bulge's column, to be reflected
                if (k > m)
                {
                    v[0] = _h[(k * _n) + (k - 1u)];
                    v[1] = _h[((k + 1u) * _n) + (k - 1u)];

                    if (three)
                    {
                        v[2] = _h[((k + 2u) * _n) + (k - 1u)];
                    }
                }

                t1 = d_internal_decomp_@S@_reflector@F@(&v[0], &v[1],
                                                        nr - 1u, 1u);

                if (k > m)
                {
                    _h[(k * _n) + (k - 1u)]        = v[0];
                    _h[((k + 1u) * _n) + (k - 1u)] = (@T@)0;

                    if (k < (i - 1u))
                    {
                        _h[((k + 2u) * _n) + (k - 1u)] = (@T@)0;
                    }
                }
                // the first, inside the block: H(k, k - 1) times 1 - t1,
                // which v's underflow cannot spoil
                else if (m > l)
                {
                    const @T@ f = (@T@)1 - t1;

                    _h[(k * _n) + (k - 1u)] = _h[(k * _n) + (k - 1u)] * f;
                }

                v2 = v[1];
                t2 = t1 * v2;

                if (three)
                {
                    v3 = v[2];
                    t3 = t1 * v3;
                }

                // from the left: rows k to k + nr - 1, columns k to i2
                for (jj = k; jj <= i2; ++jj)
                {
                    d_internal_decomp_@S@_reflect3(_h + ((k * _n) + jj), _n,
                                                   three, v2, v3, t1, t2,
                                                   t3);
                }

                // from the right: rows i1 to min(k + 3, i)
                last = three ? (((k + 3u) < i) ? (k + 3u) : i) : i;

                for (jj = i1; jj <= last; ++jj)
                {
                    d_internal_decomp_@S@_reflect3(_h + ((jj * _n) + k), 1u,
                                                   three, v2, v3, t1, t2,
                                                   t3);
                }

                // Z's columns
                if (_z != NULL)
                {
                    for (jj = 0u; jj < _n; ++jj)
                    {
                        d_internal_decomp_@S@_reflect3(_z + ((jj * _n) + k),
                                                       1u, three, v2, v3,
                                                       t1, t2, t3);
                    }
                }
            }
        }

        // one eigenvalue
        if (l == i)
        {
            d_internal_decomp_@S@_put(_values, _real, _complex, i,
                                      _h[(i * _n) + i], (@T@)0);
        }
        // a pair, the block brought to standard form
        else
        {
            struct dj_cplx_@S@ pair[2] = {
                { { (@T@)0, (@T@)0 } },
                { { (@T@)0, (@T@)0 } }
            };
            @T@                cs = (@T@)1;
            @T@                sn = (@T@)0;

            d_internal_decomp_@S@_standard_pair@F@(
                &_h[((i - 1u) * _n) + (i - 1u)],
                &_h[((i - 1u) * _n) + i],
                &_h[(i * _n) + (i - 1u)],
                &_h[(i * _n) + i],
                pair,
                &cs,
                &sn);

            d_internal_decomp_@S@_put(_values, _real, _complex, i - 1u,
                                      pair[0].part[0], pair[0].part[1]);
            d_internal_decomp_@S@_put(_values, _real, _complex, i,
                                      pair[1].part[0], pair[1].part[1]);

            if (wantt)
            {
                // the rest of rows i - 1 and i, then of the columns
                if (i2 > i)
                {
                    d_internal_decomp_@S@_rotate(
                        _h + (((i - 1u) * _n) + (i + 1u)),
                        _h + ((i * _n) + (i + 1u)),
                        i2 - i, 1u, cs, sn);
                }

                d_internal_decomp_@S@_rotate(_h + ((i1 * _n) + (i - 1u)),
                                             _h + ((i1 * _n) + i),
                                             (i - i1) - 1u, _n, cs, sn);
            }

            if (_z != NULL)
            {
                d_internal_decomp_@S@_rotate(_z + (i - 1u), _z + i, _n, _n,
                                             cs, sn);
            }
        }

        kdefl = 0u;
        top   = l;
    }

    return true;
}

// d_decomp_@S@_schur@F@
//   the eigenvalues of the n-by-n upper Hessenberg _h, rows and columns
// _ilo to _ihi its unreduced part (the rest triangular, as balancing leaves
// it), into _values, by Francis's implicit double-shift QR, as LAPACK's
// dlahqr: from the bottom up, a subdiagonal entry negligible by Ahues and
// Kressner's test - relative to its neighbours, _tol below the family's
// epsilon taken as the epsilon - set to zero and the block below it split
// off, a block of one its eigenvalue, a block of two brought to standard
// form, a complex pair exactly conjugate and a real eigenvalue's imaginary
// part exactly +0; else a double-shift step on the unreduced block from the
// row where two small subdiagonal entries let it start, its shifts the
// trailing 2-by-2's eigenvalues, a real pair as one of them twice, and at
// every tenth step without a split an exceptional shift. At most _max_iter
// steps in all. With _z NULL only the eigenvalues are found; else _h ends
// in real Schur form, quasi-triangular, its blocks of two standard, and the
// n-by-n _z, Q on entry, is multiplied by each transformation, so that
// Q^T A Q = Z T Z^T. _values receives the eigenvalues in the order of T's
// diagonal, each pair positive first, the rows outside _ilo.._ihi their
// diagonal entries. Returns whether all were found; if not, the rows not
// reached take their diagonal entries.
D_MATH_FN@RT@ bool
d_decomp_@S@_schur@F@(
    @T@*                _h,
    size_t              _n,
    size_t              _ilo,
    size_t              _ihi,
    @T@*                _z,
    struct dj_cplx_@S@* _values,
    size_t              _max_iter,
    @T@                 _tol
)
{
    return d_internal_decomp_@S@_schur@F@(_h, _n, _ilo, _ihi, _z, _values,
                                          NULL, NULL, _max_iter, _tol);
}

// d_decomp_@S@_schur_vectors@F@
//   the right eigenvectors of the n-by-n real Schur form _t, back-
// transformed by the Schur vectors, as LAPACK's dtrevc with job 'B': _vr
// holds Z on entry, and on exit the eigenvectors of Z T Z^T in LAPACK's
// storage - a real eigenvalue's vector its column, a pair's, for the
// eigenvalue of positive imaginary part, real and imaginary parts in its two
// columns - each scaled to a largest magnitude of 1 (|re| + |im| for a
// pair). From the last eigenvalue up, each by back-substitution through the
// blocks above it, (T - w I) x = 0 with x's own entry 1, each block's
// system solved by d_internal_decomp_@S@_small_solve, its pivots no smaller
// than ulp times |w|, and x scaled down wherever the next step could
// overflow; then Z x. _work is scratch of 3 n entries.
D_MATH_FN_KERNEL@RT@ void
d_decomp_@S@_schur_vectors@F@(
    const @T@* _t,
    size_t     _n,
    @T@*       _vr,
    @T@*       _work
)
{
    const @T@ ulp    = @EPS@;
    const @T@ smlnum = @MIN@ * ((@T@)_n / ulp);
    const @T@ bignum = ((@T@)1 - ulp) / smlnum;
    @T@*      norms  = _work;
    @T@*      xr     = _work + _n;
    @T@*      xi     = _work + (2u * _n);
    int       ip     = 0;
    size_t    j      = 0u;
    size_t    k      = 0u;
    size_t    r      = 0u;
    size_t    top    = 0u;

    // nothing to find
    if (_n == 0u)
    {
        return;
    }

    // each column's 1-norm above the diagonal, to foresee overflow
    norms[0] = (@T@)0;

    for (j = 1u; j < _n; ++j)
    {
        norms[j] = (@T@)0;

        for (k = 0u; k < j; ++k)
        {
            norms[j] = norms[j] + d_internal_math_@S@_abs(_t[(k * _n) + j]);
        }
    }

    for (top = _n; top > 0u; --top)
    {
        const size_t ki   = top - 1u;
        @T@          wr   = (@T@)0;
        @T@          wi   = (@T@)0;
        @T@          smin = (@T@)0;

        // the first of a pair, found with the second
        if (ip == 1)
        {
            ip = 0;

            continue;
        }

        // the second of a pair
        if ( (ki > 0u) &&
             (_t[(ki * _n) + (ki - 1u)] != (@T@)0) )
        {
            ip = -1;
        }

        wr = _t[(ki * _n) + ki];

        if (ip != 0)
        {
            const @T@ r1 = d_math_@S@_sqrt@F@(
                               d_internal_math_@S@_abs(
                                   _t[(ki * _n) + (ki - 1u)]));
            const @T@ r2 = d_math_@S@_sqrt@F@(
                               d_internal_math_@S@_abs(
                                   _t[((ki - 1u) * _n) + ki]));

            wi = r1 * r2;
        }

        {
            const @T@ a1 = d_internal_math_@S@_abs(wr);
            const @T@ a2 = d_internal_math_@S@_abs(wi);
            const @T@ s  = a1 + a2;
            const @T@ u  = ulp * s;

            smin = (u > smlnum) ? u : smlnum;
        }

        // a real eigenvector
        if (ip == 0)
        {
            size_t jnext = ki;
            size_t jj    = 0u;

            xr[ki] = (@T@)1;

            for (k = 0u; k < ki; ++k)
            {
                xr[k] = -_t[(k * _n) + ki];
            }

            // (T(0..ki-1, 0..ki-1) - wr) x = scale x, block by block
            for (jj = ki; jj > 0u; --jj)
            {
                const size_t j2    = jj - 1u;
                size_t       j1    = j2;
                @T@          x[2]  = { (@T@)0, (@T@)0 };
                @T@          scale = (@T@)1;
                @T@          xnorm = (@T@)0;

                // inside a block already solved
                if ((j2 + 1u) > jnext)
                {
                    continue;
                }

                jnext = j2;

                if ( (j2 > 0u) &&
                     (_t[(j2 * _n) + (j2 - 1u)] != (@T@)0) )
                {
                    j1    = j2 - 1u;
                    jnext = j2 - 1u;
                }

                // a block of one
                if (j1 == j2)
                {
                    d_internal_decomp_@S@_small_solve(
                        1u, 1u, smin, _t + ((j2 * _n) + j2), _n, xr + j2,
                        NULL, wr, (@T@)0, x, NULL, &scale, &xnorm);

                    // x scaled down where the update could overflow
                    if (xnorm > (@T@)1)
                    {
                        const @T@ bound = bignum / xnorm;

                        if (norms[j2] > bound)
                        {
                            x[0]  = x[0] / xnorm;
                            scale = scale / xnorm;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                        }
                    }

                    xr[j2] = x[0];

                    for (k = 0u; k < j2; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }
                }
                // a block of two
                else
                {
                    d_internal_decomp_@S@_small_solve(
                        2u, 1u, smin, _t + ((j1 * _n) + j1), _n, xr + j1,
                        NULL, wr, (@T@)0, x, NULL, &scale, &xnorm);

                    if (xnorm > (@T@)1)
                    {
                        const @T@ beta  = (norms[j1] > norms[j2])
                                              ? norms[j1] : norms[j2];
                        const @T@ bound = bignum / xnorm;

                        if (beta > bound)
                        {
                            x[0]  = x[0] / xnorm;
                            x[1]  = x[1] / xnorm;
                            scale = scale / xnorm;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                        }
                    }

                    xr[j1] = x[0];
                    xr[j2] = x[1];

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j1];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[1] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }
                }
            }

            // Z x, into the column
            if (ki > 0u)
            {
                const @T@ beta = xr[ki];

                for (r = 0u; r < _n; ++r)
                {
                    if (beta == (@T@)0)
                    {
                        _vr[(r * _n) + ki] = (@T@)0;
                    }
                    else if (beta != (@T@)1)
                    {
                        _vr[(r * _n) + ki] = beta * _vr[(r * _n) + ki];
                    }
                }

                for (k = 0u; k < ki; ++k)
                {
                    const @T@ temp = xr[k];

                    for (r = 0u; r < _n; ++r)
                    {
                        const @T@ p = temp * _vr[(r * _n) + k];

                        _vr[(r * _n) + ki] = _vr[(r * _n) + ki] + p;
                    }
                }
            }

            // the largest magnitude 1
            {
                size_t    best = 0u;
                @T@       remax = (@T@)0;

                for (r = 1u; r < _n; ++r)
                {
                    if (d_internal_math_@S@_abs(_vr[(r * _n) + ki]) >
                        d_internal_math_@S@_abs(_vr[(best * _n) + ki]))
                    {
                        best = r;
                    }
                }

                remax = (@T@)1 /
                        d_internal_math_@S@_abs(_vr[(best * _n) + ki]);

                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + ki] = remax * _vr[(r * _n) + ki];
                }
            }
        }
        // a complex pair's eigenvector, for wr + wi i
        else
        {
            size_t jnext = ki - 1u;
            size_t jj    = 0u;

            // the block's own: [T(ki-1..ki, ki-1..ki) - w] x = 0
            if (d_internal_math_@S@_abs(_t[((ki - 1u) * _n) + ki]) >=
                d_internal_math_@S@_abs(_t[(ki * _n) + (ki - 1u)]))
            {
                xr[ki - 1u] = (@T@)1;
                xi[ki]      = wi / _t[((ki - 1u) * _n) + ki];
            }
            else
            {
                xr[ki - 1u] = -wi / _t[(ki * _n) + (ki - 1u)];
                xi[ki]      = (@T@)1;
            }

            xr[ki]      = (@T@)0;
            xi[ki - 1u] = (@T@)0;

            for (k = 0u; (k + 1u) < ki; ++k)
            {
                xr[k] = -xr[ki - 1u] * _t[(k * _n) + (ki - 1u)];
                xi[k] = -xi[ki] * _t[(k * _n) + ki];
            }

            // (T(0..ki-2, 0..ki-2) - w) x = scale x, block by block
            for (jj = ki - 1u; jj > 0u; --jj)
            {
                const size_t j2    = jj - 1u;
                size_t       j1    = j2;
                @T@          x[2]  = { (@T@)0, (@T@)0 };
                @T@          y[2]  = { (@T@)0, (@T@)0 };
                @T@          scale = (@T@)1;
                @T@          xnorm = (@T@)0;

                // inside a block already solved
                if ((j2 + 1u) > jnext)
                {
                    continue;
                }

                jnext = j2;

                if ( (j2 > 0u) &&
                     (_t[(j2 * _n) + (j2 - 1u)] != (@T@)0) )
                {
                    j1    = j2 - 1u;
                    jnext = j2 - 1u;
                }

                // a block of one
                if (j1 == j2)
                {
                    d_internal_decomp_@S@_small_solve(
                        1u, 2u, smin, _t + ((j2 * _n) + j2), _n, xr + j2,
                        xi + j2, wr, wi, x, y, &scale, &xnorm);

                    if (xnorm > (@T@)1)
                    {
                        const @T@ bound = bignum / xnorm;

                        if (norms[j2] > bound)
                        {
                            x[0]  = x[0] / xnorm;
                            y[0]  = y[0] / xnorm;
                            scale = scale / xnorm;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                            xi[k] = scale * xi[k];
                        }
                    }

                    xr[j2] = x[0];
                    xi[j2] = y[0];

                    for (k = 0u; k < j2; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j2; ++k)
                    {
                        const @T@ p = -y[0] * _t[(k * _n) + j2];

                        xi[k] = xi[k] + p;
                    }
                }
                // a block of two
                else
                {
                    d_internal_decomp_@S@_small_solve(
                        2u, 2u, smin, _t + ((j1 * _n) + j1), _n, xr + j1,
                        xi + j1, wr, wi, x, y, &scale, &xnorm);

                    if (xnorm > (@T@)1)
                    {
                        const @T@ beta  = (norms[j1] > norms[j2])
                                              ? norms[j1] : norms[j2];
                        const @T@ bound = bignum / xnorm;

                        if (beta > bound)
                        {
                            const @T@ rec = (@T@)1 / xnorm;

                            x[0]  = x[0] * rec;
                            y[0]  = y[0] * rec;
                            x[1]  = x[1] * rec;
                            y[1]  = y[1] * rec;
                            scale = scale * rec;
                        }
                    }

                    if (scale != (@T@)1)
                    {
                        for (k = 0u; k <= ki; ++k)
                        {
                            xr[k] = scale * xr[k];
                            xi[k] = scale * xi[k];
                        }
                    }

                    xr[j1] = x[0];
                    xr[j2] = x[1];
                    xi[j1] = y[0];
                    xi[j2] = y[1];

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[0] * _t[(k * _n) + j1];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -x[1] * _t[(k * _n) + j2];

                        xr[k] = xr[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -y[0] * _t[(k * _n) + j1];

                        xi[k] = xi[k] + p;
                    }

                    for (k = 0u; k < j1; ++k)
                    {
                        const @T@ p = -y[1] * _t[(k * _n) + j2];

                        xi[k] = xi[k] + p;
                    }
                }
            }

            // Z x, into the two columns
            if (ki > 1u)
            {
                const @T@ beta_r = xr[ki - 1u];
                const @T@ beta_i = xi[ki];

                for (r = 0u; r < _n; ++r)
                {
                    if (beta_r == (@T@)0)
                    {
                        _vr[(r * _n) + (ki - 1u)] = (@T@)0;
                    }
                    else if (beta_r != (@T@)1)
                    {
                        _vr[(r * _n) + (ki - 1u)] =
                            beta_r * _vr[(r * _n) + (ki - 1u)];
                    }

                    if (beta_i == (@T@)0)
                    {
                        _vr[(r * _n) + ki] = (@T@)0;
                    }
                    else if (beta_i != (@T@)1)
                    {
                        _vr[(r * _n) + ki] = beta_i * _vr[(r * _n) + ki];
                    }
                }

                for (k = 0u; (k + 1u) < ki; ++k)
                {
                    const @T@ temp = xr[k];

                    for (r = 0u; r < _n; ++r)
                    {
                        const @T@ p = temp * _vr[(r * _n) + k];

                        _vr[(r * _n) + (ki - 1u)] =
                            _vr[(r * _n) + (ki - 1u)] + p;
                    }
                }

                for (k = 0u; (k + 1u) < ki; ++k)
                {
                    const @T@ temp = xi[k];

                    for (r = 0u; r < _n; ++r)
                    {
                        const @T@ p = temp * _vr[(r * _n) + k];

                        _vr[(r * _n) + ki] = _vr[(r * _n) + ki] + p;
                    }
                }
            }
            else
            {
                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + (ki - 1u)] =
                        xr[ki - 1u] * _vr[(r * _n) + (ki - 1u)];
                    _vr[(r * _n) + ki] = xi[ki] * _vr[(r * _n) + ki];
                }
            }

            // the largest |re| + |im| 1
            {
                @T@ emax  = (@T@)0;
                @T@ remax = (@T@)0;

                for (r = 0u; r < _n; ++r)
                {
                    const @T@ a1 = d_internal_math_@S@_abs(
                                       _vr[(r * _n) + (ki - 1u)]);
                    const @T@ a2 = d_internal_math_@S@_abs(
                                       _vr[(r * _n) + ki]);
                    const @T@ s  = a1 + a2;

                    if (s > emax)
                    {
                        emax = s;
                    }
                }

                remax = (@T@)1 / emax;

                for (r = 0u; r < _n; ++r)
                {
                    _vr[(r * _n) + (ki - 1u)] =
                        remax * _vr[(r * _n) + (ki - 1u)];
                    _vr[(r * _n) + ki] = remax * _vr[(r * _n) + ki];
                }
            }
        }

        // past the second of a pair: its first comes next, and is done
        if (ip == -1)
        {
            ip = 1;
        }
    }

    return;
}

// d_internal_decomp_@S@_cnorm@F@
//   the Euclidean norm of the _count complex entries _x[0], _x[_stride],
// ...: the root of the sum of their parts' squares, scaled where that
// passes the range or underflows as d_internal_decomp_@S@_norm@F@ scales
// it (S256).
D_MATH_FN@RT@ @T@
d_internal_decomp_@S@_cnorm@F@(
    const struct dj_cplx_@S@* _x,
    size_t                    _count,
    size_t                    _stride
)
{
    const @T@ small   = d_math_@S@_sqrt@F@(@MIN@ / @EPS@);
    @T@       sum     = (@T@)0;
    @T@       norm    = (@T@)0;
    @T@       largest = (@T@)0;
    size_t    i       = 0u;
    size_t    p       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        for (p = 0u; p < 2u; ++p)
        {
            const @T@ v  = _x[i * _stride].part[p];
            const @T@ sq = v * v;

            sum = sum + sq;
        }
    }

    norm = d_math_@S@_sqrt@F@(sum);

    // in range: the sum's root
    if ( (norm <= @MAX@) &&
         (!(norm < small)) )
    {
        return norm;
    }

    // the largest magnitude; a NaN or an infinity, the sum's root
    for (i = 0u; i < _count; ++i)
    {
        for (p = 0u; p < 2u; ++p)
        {
            const @T@ v = d_internal_math_@S@_abs(_x[i * _stride].part[p]);

            if (!(v <= @MAX@))
            {
                return norm;
            }

            if (v > largest)
            {
                largest = v;
            }
        }
    }

    // the zero vector
    if (largest == (@T@)0)
    {
        return norm;
    }

    {
        @T@ pre   = (@T@)1;
        @T@ power = (@T@)1;

        // a subnormal largest: lifted into the normal range first
        if (largest < @MIN@)
        {
            pre     = d_internal_math_@S@_pow2(@P@);
            largest = largest * pre;
        }

        {
            const struct dj_internal_math_@S@_binade b =
                d_internal_math_@S@_binade_of(largest);

            power = b.power;
        }

        sum = (@T@)0;

        for (i = 0u; i < _count; ++i)
        {
            for (p = 0u; p < 2u; ++p)
            {
                const @T@ lifted = _x[i * _stride].part[p] * pre;
                const @T@ v      = lifted / power;
                const @T@ sq     = v * v;

                sum = sum + sq;
            }
        }

        {
            const @T@ root     = d_math_@S@_sqrt@F@(sum);
            const @T@ restored = root * power;

            return restored / pre;
        }
    }
}

// d_internal_decomp_@S@_unit_vectors@F@
//   the complex eigenvectors of the real Schur form _t, from _vr as
// d_decomp_@S@_schur_vectors@F@ and d_decomp_@S@_balance_back
// leave them, into the n-by-n _vectors, column for column: each of unit
// length, its entry of largest modulus (the first among equals) real and
// positive, a real eigenvalue's vector real, its imaginary parts +0, and
// the second of a pair the first's conjugate.
D_MATH_FN_KERNEL@RT@ void
d_internal_decomp_@S@_unit_vectors@F@(
    const @T@*          _t,
    const @T@*          _vr,
    size_t              _n,
    struct dj_cplx_@S@* _vectors
)
{
    size_t j = 0u;
    size_t r = 0u;

    while (j < _n)
    {
        size_t best = 0u;

        // a real eigenvalue's vector
        if ( ((j + 1u) == _n) ||
             (_t[((j + 1u) * _n) + j] == (@T@)0) )
        {
            const @T@ norm = d_internal_decomp_@S@_norm@F@(_vr + j, _n,
                                                           _n);
            bool      flip = false;

            for (r = 1u; r < _n; ++r)
            {
                if (d_internal_math_@S@_abs(_vr[(r * _n) + j]) >
                    d_internal_math_@S@_abs(_vr[(best * _n) + j]))
                {
                    best = r;
                }
            }

            flip = (_vr[(best * _n) + j] < (@T@)0);

            for (r = 0u; r < _n; ++r)
            {
                const @T@ unit = _vr[(r * _n) + j] / norm;

                _vectors[(r * _n) + j].part[0] = flip ? -unit : unit;
                _vectors[(r * _n) + j].part[1] = (@T@)0;
            }

            ++j;

            continue;
        }

        // a pair: columns j and j + 1 the first's real and imaginary parts
        {
            @T@ largest = (@T@)0;
            @T@ modulus = (@T@)0;
            @T@ norm    = (@T@)0;
            @T@ re      = (@T@)0;
            @T@ im      = (@T@)0;

            for (r = 0u; r < _n; ++r)
            {
                const @T@ a  = _vr[(r * _n) + j];
                const @T@ b  = _vr[(r * _n) + (j + 1u)];
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

            re      = _vr[(best * _n) + j];
            im      = _vr[(best * _n) + (j + 1u)];
            modulus = d_internal_decomp_@S@_hypot@F@(re, im);

            // turned so that the largest entry is real and positive
            {
                const @T@                ur = re / modulus;
                const @T@                ui = im / modulus;
                const struct dj_cplx_@S@ u  = d_cplx_@S@_make(ur, -ui);

                for (r = 0u; r < _n; ++r)
                {
                    if (r == best)
                    {
                        _vectors[(r * _n) + j] =
                            d_cplx_@S@_make(modulus, (@T@)0);
                    }
                    else
                    {
                        _vectors[(r * _n) + j] = d_cplx_@S@_mul(
                            d_cplx_@S@_make(_vr[(r * _n) + j],
                                            _vr[(r * _n) + (j + 1u)]),
                            u);
                    }
                }
            }

            // unit length
            norm = d_internal_decomp_@S@_cnorm@F@(_vectors + j, _n, _n);

            for (r = 0u; r < _n; ++r)
            {
                const @T@ a = _vectors[(r * _n) + j].part[0] / norm;
                const @T@ b = _vectors[(r * _n) + j].part[1] / norm;

                _vectors[(r * _n) + j].part[0]        = a;
                _vectors[(r * _n) + j].part[1]        = b;
                _vectors[(r * _n) + (j + 1u)].part[0] = a;
                _vectors[(r * _n) + (j + 1u)].part[1] = -b;
            }

            _vectors[(best * _n) + j].part[1]        = (@T@)0;
            _vectors[(best * _n) + (j + 1u)].part[1] = (@T@)0;

            j += 2u;
        }
    }

    return;
}

// d_decomp_@S@_eigen@F@
//   the eigenvalues of the general n-by-n _a and, unless _vectors is NULL,
// an eigenvector for each, as LAPACK's dgeev finds them: _a scaled by a
// power of two to a largest magnitude in [1, 2), exactly, so that 2^k A's
// eigenvalues are 2^k times A's bit for bit (barring overflow and
// underflow); balanced (d_decomp_@S@_balance@F@); reduced to Hessenberg
// form (d_decomp_@S@_hessenberg_q@F@); its real Schur form found by
// Francis's double-shift QR (d_decomp_@S@_schur@F@), at most _max_iter
// steps, _tol its deflation's tolerance (the epsilon where smaller); and
// each eigenvector from the Schur form by back-substitution, back-
// transformed, of unit length with its entry of largest modulus real and
// positive (the first among equals). _values receives the eigenvalues,
// ascending by real part, then by imaginary part: a complex pair exactly
// conjugate, side by side, negative first where no real eigenvalue shares
// their real part, a real eigenvalue's imaginary part exactly +0; and
// column j of the row-major n-by-n _vectors the eigenvector of _values[j],
// a real eigenvalue's real, a pair's conjugate. _a is consumed; _work is
// scratch of n (n + 4) entries, or n where _vectors is NULL; _perm of n,
// unused where _vectors is NULL. Returns whether every eigenvalue was found.
// If not, those found are given and the rest the diagonal's entries, and
// every entry of _vectors is a NaN; a NaN or an infinity in _a gives every
// eigenvalue and entry a NaN, and returns false; the zero matrix has zero
// eigenvalues and the identity's columns.
D_MATH_FN_KERNEL@RT@ bool
d_decomp_@S@_eigen@F@(
    @T@*                _a,
    size_t              _n,
    struct dj_cplx_@S@* _values,
    struct dj_cplx_@S@* _vectors,
    @T@*                _work,
    size_t*             _perm,
    size_t              _max_iter,
    @T@                 _tol
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_decomp_@S@_scaling_of(_a, _n * _n);
    @T@*   scale     = _work;
    @T@*   z         = NULL;
    size_t ilo       = 0u;
    size_t ihi       = 0u;
    size_t i         = 0u;
    size_t k         = 0u;
    bool   converged = true;

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
            _values[i].part[0] = d_internal_math_@S@_nan();
            _values[i].part[1] = d_internal_math_@S@_nan();
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i].part[0] = d_internal_math_@S@_nan();
                _vectors[i].part[1] = d_internal_math_@S@_nan();
            }
        }

        return false;
    }

    // the zero matrix: zeros, and the identity's columns
    if (sc.zero)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i].part[0] = (@T@)0;
            _values[i].part[1] = (@T@)0;
        }

        if (_vectors != NULL)
        {
            for (i = 0u; i < _n; ++i)
            {
                for (k = 0u; k < _n; ++k)
                {
                    _vectors[(i * _n) + k].part[0] =
                        (i == k) ? (@T@)1 : (@T@)0;
                    _vectors[(i * _n) + k].part[1] = (@T@)0;
                }
            }
        }

        return true;
    }

    // the largest magnitude into [1, 2), exactly
    for (i = 0u; i < (_n * _n); ++i)
    {
        const @T@ lifted = _a[i] * sc.pre;

        _a[i] = lifted / sc.power;
    }

    d_decomp_@S@_balance@F@(_a, _n, _perm, scale, &ilo, &ihi);

    // Q, then Z, from the identity
    if (_vectors != NULL)
    {
        z = _work + _n;

        for (i = 0u; i < _n; ++i)
        {
            for (k = 0u; k < _n; ++k)
            {
                z[(i * _n) + k] = (i == k) ? (@T@)1 : (@T@)0;
            }
        }
    }

    d_decomp_@S@_hessenberg_q@F@(_a, _n, ilo, ihi, z);

    converged = d_internal_decomp_@S@_schur@F@(_a, _n, ilo, ihi, z, _values,
                                               NULL, NULL, _max_iter, _tol);

    // the eigenvectors, or NaNs where the iteration stopped short
    if (_vectors != NULL)
    {
        if (converged)
        {
            d_decomp_@S@_schur_vectors@F@(_a, _n, z,
                                                   z + (_n * _n));
            d_decomp_@S@_balance_back(z, _n, _n, _perm, scale, ilo, ihi);
            d_internal_decomp_@S@_unit_vectors@F@(_a, z, _n, _vectors);
        }
        else
        {
            for (i = 0u; i < (_n * _n); ++i)
            {
                _vectors[i].part[0] = d_internal_math_@S@_nan();
                _vectors[i].part[1] = d_internal_math_@S@_nan();
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

// d_decomp_@S@_eigenvalues@F@
//   the eigenvalues of the general n-by-n _h, ascending, into _values, as
// d_decomp_@S@_eigen@F@ finds them - scaled by a power of two, balanced,
// reduced to Hessenberg form, and Francis's double-shift QR, at most
// _max_iter steps, _tol its deflation's tolerance (the epsilon where
// smaller) - their real parts: the same bits as its real eigenvalues.
// _h is consumed. Returns whether every eigenvalue was found and real. A
// complex pair gives its real part twice, the rest of the spectrum still
// found; a spent budget gives the diagonal for what is left unsolved; a NaN
// or an infinity in _h gives NaN for every eigenvalue. All three return
// false.
D_MATH_FN@RT@ bool
d_decomp_@S@_eigenvalues@F@(
    @T@*   _h,
    size_t _n,
    @T@*   _values,
    size_t _max_iter,
    @T@    _tol
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_decomp_@S@_scaling_of(_h, _n * _n);
    size_t ilo       = 0u;
    size_t ihi       = 0u;
    size_t i         = 0u;
    bool   converged = true;
    bool   complex   = false;

    // a NaN or an infinity: no eigenvalue
    if (!sc.finite)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = d_internal_math_@S@_nan();
        }

        return false;
    }

    // the zero matrix
    if (sc.zero)
    {
        for (i = 0u; i < _n; ++i)
        {
            _values[i] = (@T@)0;
        }

        return true;
    }

    // the largest magnitude into [1, 2), exactly
    for (i = 0u; i < (_n * _n); ++i)
    {
        const @T@ lifted = _h[i] * sc.pre;

        _h[i] = lifted / sc.power;
    }

    // balanced: _values holds the scaling until the eigenvalues come
    d_decomp_@S@_balance@F@(_h, _n, NULL, _values, &ilo, &ihi);
    d_decomp_@S@_hessenberg_q@F@(_h, _n, ilo, ihi, NULL);

    converged = d_internal_decomp_@S@_schur@F@(_h, _n, ilo, ihi, NULL, NULL,
                                               _values, &complex, _max_iter,
                                               _tol);

    // back to the matrix's scale: exact unless a value overflows or
    // underflows
    for (i = 0u; i < _n; ++i)
    {
        const @T@ restored = _values[i] * sc.power;

        _values[i] = restored / sc.pre;
    }

    // ascending
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
        }
    }

    return ( (converged) &&
             (!complex) );
}

// d_decomp_@S@_eigenvector@F@
//   an eigenvector of the n-by-n _a for its eigenvalue _lambda into _x, of
// unit length, its largest entry (the first, among equals) positive, by
// inverse iteration on _a scaled as d_decomp_@S@_eigenvalues@F@ scales it:
// shifted past the scaled eigenvalue by 4 epsilon times its magnitude (at
// least 4 epsilon), so that the shift differs from it in the family's own
// precision, and moved 16 times further, up to three times, while the
// shifted matrix is exactly singular; then U x = e solved first, e all
// ones, as LAPACK's dlaein does - in effect a start of P^T L e, which
// follows the shifted matrix, where any fixed start, e itself among them,
// is orthogonal to some eigenvectors - and two full solves, each iterate
// divided by its largest magnitude. _lu (n by n), _perm (n) and _y (n) are
// scratch. Returns false where there is nothing to find - _a or _lambda
// not finite, _a zero, the shifted matrix singular at every shift tried, or
// an iterate overflowing or vanishing - and _x is then no eigenvector.
D_MATH_FN@RT@ bool
d_decomp_@S@_eigenvector@F@(
    const @T@* _a,
    size_t     _n,
    @T@        _lambda,
    @T@*       _lu,
    size_t*    _perm,
    @T@*       _x,
    @T@*       _y
)
{
    const struct dj_internal_decomp_@S@_scaling sc =
        d_internal_decomp_@S@_scaling_of(_a, _n * _n);
    const @T@ four_eps = (@T@)4 * @EPS@;
    @T@       sign     = (@T@)1;
    @T@       lam      = (@T@)0;
    @T@       delta    = (@T@)0;
    bool   singular = true;
    size_t attempt  = 0u;
    size_t step     = 0u;
    size_t i        = 0u;

    // nothing to find: a NaN or an infinity, the zero matrix, or no value
    if ( (!sc.finite) ||
         (sc.zero) ||
         (_lambda != _lambda) ||
         (d_internal_math_@S@_abs(_lambda) > @MAX@) )
    {
        return false;
    }

    {
        const @T@ lifted = _lambda * sc.pre;

        lam = lifted / sc.power;

        {
            const @T@ mag = d_internal_math_@S@_abs(lam);

            delta = four_eps * ((mag > (@T@)1) ? mag : (@T@)1);
        }
    }

    // A - (lambda + delta) I, scaled; a shift moved on while exactly
    // singular
    for (attempt = 0u; (attempt < 4u) && singular; ++attempt)
    {
        const @T@ shift = lam + delta;

        for (i = 0u; i < (_n * _n); ++i)
        {
            const @T@ lifted = _a[i] * sc.pre;

            _lu[i] = lifted / sc.power;
        }

        for (i = 0u; i < _n; ++i)
        {
            _lu[(i * _n) + i] = _lu[(i * _n) + i] - shift;
        }

        singular = d_decomp_@S@_lu(_lu, _n, _perm, &sign);
        delta    = delta * (@T@)16;
    }

    // singular at every shift
    if (singular)
    {
        return false;
    }

    // the first step: U x = e
    for (i = 0u; i < _n; ++i)
    {
        _y[i] = (@T@)1;
    }

    d_decomp_@S@_back_sub(_lu, _n, _y, _x);

    for (step = 0u; step < 3u; ++step)
    {
        @T@ largest = (@T@)0;

        // the iterate's largest magnitude: a NaN or an infinity ends it
        for (i = 0u; i < _n; ++i)
        {
            const @T@ v = d_internal_math_@S@_abs(_x[i]);

            if ( (v != v) ||
                 (v > @MAX@) )
            {
                return false;
            }

            if (v > largest)
            {
                largest = v;
            }
        }

        // the zero vector: no direction
        if (largest == (@T@)0)
        {
            return false;
        }

        for (i = 0u; i < _n; ++i)
        {
            _x[i] = _x[i] / largest;
        }

        // two full solves after the first
        if (step < 2u)
        {
            d_decomp_@S@_lu_solve(_lu, _perm, _n, _x, _y);

            for (i = 0u; i < _n; ++i)
            {
                _x[i] = _y[i];
            }
        }
    }

    {
        @T@    norm_sq = (@T@)0;
        @T@    norm    = (@T@)0;
        size_t top     = 0u;

        for (i = 0u; i < _n; ++i)
        {
            const @T@ sq = _x[i] * _x[i];

            norm_sq = norm_sq + sq;

            if (d_internal_math_@S@_abs(_x[i]) >
                d_internal_math_@S@_abs(_x[top]))
            {
                top = i;
            }
        }

        norm = d_math_@S@_sqrt@F@(norm_sq);

        {
            const bool flip = (_x[top] < (@T@)0);

            // unit length, the largest entry positive
            for (i = 0u; i < _n; ++i)
            {
                const @T@ unit = _x[i] / norm;

                _x[i] = flip ? -unit : unit;
            }
        }
    }

    return true;
}
"""

DECOMP_POP = r"""
// gcc's -Warray-bounds, restored
#if ( defined(__GNUC__) &&                                                    \
      !defined(__clang__) )
    #pragma GCC diagnostic pop
#endif
"""


DECOMP_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  LU, with partial pivoting
    2.  QR, the SVD, the symmetric eigensolver and Cholesky
    3.  The eigenvalues and eigenvectors of a general matrix
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  LU, with partial pivoting
    2.  QR, the SVD, the symmetric eigensolver and Cholesky
    3.  The eigenvalues and eigenvectors of a general matrix
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  LU, with partial pivoting
    2.  QR, the SVD, the symmetric eigensolver and Cholesky
    3.  The eigenvalues and eigenvectors of a general matrix
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_DECOMP_H
#define DJINTERP_C_MATH_DECOMP_H 1

// std
#include <float.h>           // FLT_EPSILON ... LDBL_MAX: the families'
                             // limits
#include <stddef.h>          // size_t
// djinterp
#include "./cplx.h"          // struct dj_cplx_*, d_cplx_*_make, _mul and
                             // _div: the eigenvalues and eigenvectors
#include "./elementary.h"    // d_math_*_sqrt and its _c form,
                             // d_internal_math_*_abs, d_internal_math_*_nan
#include "./float_order.h"   // d_internal_math_*_binade_of,
                             // d_internal_math_*_pow2
#include "./math_common.h"   // D_MATH_FN
"""

DECOMP_TAIL = r"""

#endif  // DJINTERP_C_MATH_DECOMP_H
"""

DECOMP_OVERLOADS = [
    ("D_MATH_FN", "bool", "d_decomp_lu", [("@T@*", "_lu"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_sign")]),
    ("D_MATH_FN", "void", "d_decomp_lu_solve", [("const @T@*", "_lu"), ("const size_t*", "_perm"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "@T@", "d_decomp_lu_det", [("const @T@*", "_lu"), ("size_t", "_n"), ("@T@", "_sign")]),
    ("D_MATH_FN", "void", "d_decomp_cholesky_solve", [("const @T@*", "_l"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "@T@", "d_decomp_cholesky_det", [("const @T@*", "_l"), ("size_t", "_n")]),
    ("D_MATH_FN", "void", "d_decomp_forward_sub", [("const @T@*", "_l"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "void", "d_decomp_back_sub", [("const @T@*", "_u"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_x")]),
    ("D_MATH_FN", "void", "d_decomp_qt_mul", [("const @T@*", "_q"), ("size_t", "_m"), ("size_t", "_n"), ("const @T@*", "_b"), ("@T@*", "_c")]),
    ("D_MATH_FN", "void", "d_decomp_balance_back", [("@T@*", "_v"), ("size_t", "_n"), ("size_t", "_m"), ("const size_t*", "_perm"), ("const @T@*", "_scale"), ("size_t", "_ilo"), ("size_t", "_ihi")]),
]
for _f, _m in (("_c", "D_MATH_FN"), ("", "D_MATH_FN_RT")):
    DECOMP_OVERLOADS += [
        (_m, "void", "d_decomp_qr" + _f, [("@T@*", "_q"), ("@T@*", "_r"), ("size_t", "_m"), ("size_t", "_n"), ("@T@*", "_v")]),
        (_m, "bool", "d_decomp_cholesky" + _f, [("@T@*", "_l"), ("const @T@*", "_a"), ("size_t", "_n")]),
        (_m, "void", "d_decomp_svd" + _f, [("@T@*", "_work"), ("@T@*", "_u"), ("@T@*", "_s"), ("@T@*", "_v"), ("size_t", "_m"), ("size_t", "_n"), ("@T@", "_tol")]),
        (_m, "void", "d_decomp_sym_eigen" + _f, [("@T@*", "_work"), ("@T@*", "_values"), ("@T@*", "_vectors"), ("size_t", "_n"), ("@T@", "_threshold")]),
        (_m, "void", "d_decomp_hessenberg" + _f, [("@T@*", "_h"), ("size_t", "_n")]),
        (_m, "bool", "d_decomp_eigenvalues" + _f, [("@T@*", "_h"), ("size_t", "_n"), ("@T@*", "_values"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
        (_m, "bool", "d_decomp_eigenvector" + _f, [("const @T@*", "_a"), ("size_t", "_n"), ("@T@", "_lambda"), ("@T@*", "_lu"), ("size_t*", "_perm"), ("@T@*", "_x"), ("@T@*", "_y")]),
        (_m, "void", "d_decomp_balance" + _f, [("@T@*", "_a"), ("size_t", "_n"), ("size_t*", "_perm"), ("@T@*", "_scale"), ("size_t*", "_ilo"), ("size_t*", "_ihi")]),
        (_m, "void", "d_decomp_hessenberg_q" + _f, [("@T@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("@T@*", "_q")]),
        (_m, "bool", "d_decomp_schur" + _f, [("@T@*", "_h"), ("size_t", "_n"), ("size_t", "_ilo"), ("size_t", "_ihi"), ("@T@*", "_z"), ("struct dj_cplx_@S@*", "_values"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
        (_m, "void", "d_decomp_schur_vectors" + _f, [("const @T@*", "_t"), ("size_t", "_n"), ("@T@*", "_vr"), ("@T@*", "_work")]),
        (_m, "bool", "d_decomp_eigen" + _f, [("@T@*", "_a"), ("size_t", "_n"), ("struct dj_cplx_@S@*", "_values"), ("struct dj_cplx_@S@*", "_vectors"), ("@T@*", "_work"), ("size_t*", "_perm"), ("size_t", "_max_iter"), ("@T@", "_tol")]),
    ]


def decomp():
    lines = [
        "Decompositions: the C core of linear_algebra/decomposition.hpp,",
        "solve.hpp, svd.hpp and eigen.hpp - LU with partial pivoting and its",
        "solve and determinant, Householder QR, Cholesky and its solve and",
        "determinant, triangular substitution, Q^T b for least squares, the",
        "one-sided Jacobi SVD and the cyclic Jacobi symmetric eigensolver, and",
        "for a general matrix LAPACK's path to its eigenvalues and",
        "eigenvectors: balancing, the Hessenberg form, the real Schur form by",
        "Francis's double-shift QR, the eigenvectors from it, the eigenvalues",
        "complex numbers of cplx.h - and an eigenvector by inverse iteration,",
        "for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Matrices are row-major arrays, the faces' matrices' storage. LU and",
        "the solves need no root and come once; what takes a root comes twice,",
        "as the elementary functions do: the _c form a constant expression from",
        "C++14, the plain one on the C library's root.",
    ]
    text = banner("decomp.h", lines, DATE, "2026.11.21")
    text += "\n" + DECOMP_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        forms = ""
        eigen_forms = ""
        for f, rt in (("_c", ""), ("", "_RT")):
            forms += DECOMP_FORMS.replace("@F@", f).replace("@RT@", rt)
            eigen_forms += DECOMP_EIGEN_FORMS.replace("@F@", f).replace("@RT@", rt)
        body = DECOMP_TEMPLATE.replace("@FORMS@", forms)
        body = body.replace("@EIGEN_FORMS@", eigen_forms + DECOMP_POP)
        body = body.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", DECOMP_OVERLOADS)
    text += DECOMP_TAIL
    return text
