"""gen_c_math.mat - the text of mat.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section)


# -----------------------------------------------------------------------------
# mat.h
# -----------------------------------------------------------------------------

MAT_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// Row-major: entry (i, j) of a matrix with _cols columns is [i * _cols + j].
// A product's output must not be either input; every other output may be.
// Entry-wise operations are vec.h's over the rows * cols entries.


// @SECTION@.1    Construction
//------------------------------------------------------------------------------

D_MATH_FN void
d_mat_@S@_identity(
    @T@*   _out,
    size_t _n
)
{
    size_t i = 0u;

    d_vec_@S@_fill(_out, _n * _n, (@T@)0);

    for (i = 0u; i < _n; ++i)
    {
        _out[(i * _n) + i] = (@T@)1;
    }

    return;
}

D_MATH_FN void
d_mat_@S@_diagonal(
    @T@*       _out,
    const @T@* _diagonal,
    size_t     _n
)
{
    size_t i = 0u;

    d_vec_@S@_fill(_out, _n * _n, (@T@)0);

    for (i = 0u; i < _n; ++i)
    {
        _out[(i * _n) + i] = _diagonal[i];
    }

    return;
}

// @SECTION@.2    Rows, columns and the transpose
//------------------------------------------------------------------------------

D_MATH_FN void
d_mat_@S@_row(
    @T@*       _out,
    const @T@* _a,
    size_t     _i,
    size_t     _cols
)
{
    size_t j = 0u;

    for (j = 0u; j < _cols; ++j)
    {
        _out[j] = _a[(_i * _cols) + j];
    }

    return;
}

D_MATH_FN void
d_mat_@S@_col(
    @T@*       _out,
    const @T@* _a,
    size_t     _j,
    size_t     _rows,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        _out[i] = _a[(i * _cols) + _j];
    }

    return;
}

// d_mat_@S@_transpose
//   _out (_cols x _rows) is _a (_rows x _cols) transposed; _out must not be
// _a.
D_MATH_FN void
d_mat_@S@_transpose(
    @T@*       _out,
    const @T@* _a,
    size_t     _rows,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _cols; ++j)
        {
            _out[(j * _rows) + i] = _a[(i * _cols) + j];
        }
    }

    return;
}

// @SECTION@.3    Products
//------------------------------------------------------------------------------
// Each entry is its dot product rounded once from Dot2's pair (vec.h's
// compensated sums, S404), as accurate as the dot product computed in twice
// the precision and rounded once. The operands are tested once for the
// range where Dekker's products are exact; where both lie in it, each entry
// is summed with no test of its own, and elsewhere each entry tests and
// scales its own row and column, and is the formula as written where one
// has a NaN or an infinity. The _fast forms are the formulas as written,
// in order of k, each product rounded before it is summed. _out must not
// be an input.

// d_mat_@S@_multiply
//   _out (_rows x _cols) = _a (_rows x _inner) * _b (_inner x _cols), each
// entry rounded once from Dot2's pair.
D_MATH_FN void
d_mat_@S@_multiply(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _rows,
    size_t     _inner,
    size_t     _cols
)
{
    const bool in_range =
        ( (d_internal_vec_@S@_reach(_a, 1u, _rows * _inner) == 0) &&
          (d_internal_vec_@S@_reach(_b, 1u, _inner * _cols) == 0) );
    const @T@  zero     = (@T@)0;
    size_t     i        = 0u;

    // each entry, row i of _a by column j of _b
    for (i = 0u; i < _rows; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _cols; ++j)
        {
            const @T@* row = _a + (i * _inner);
            const @T@* col = _b + j;

            // in range: no test of its own
            if (in_range)
            {
                _out[(i * _cols) + j] =
                    d_internal_vec_@S@_dot2_direct(false, row, 1u, col, _cols,
                                                   _inner);
            }
            else
            {
                _out[(i * _cols) + j] =
                    d_internal_vec_@S@_dot2(zero, false, row, 1u, col, _cols,
                                            col, _cols, false, _inner);
            }
        }
    }

    return;
}

// d_mat_@S@_multiply_fast
//   _out (_rows x _cols) = _a (_rows x _inner) * _b (_inner x _cols) as
// written: each entry its products rounded, then added, in order of k.
D_MATH_FN void
d_mat_@S@_multiply_fast(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _rows,
    size_t     _inner,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _cols; ++j)
        {
            @T@    acc = (@T@)0;
            size_t k   = 0u;

            for (k = 0u; k < _inner; ++k)
            {
                const @T@ product = _a[(i * _inner) + k] * _b[(k * _cols) + j];

                acc += product;
            }

            _out[(i * _cols) + j] = acc;
        }
    }

    return;
}

// d_mat_@S@_multiply_vector
//   _out (_rows) = _a (_rows x _cols) * _v (_cols), each entry rounded once
// from Dot2's pair. _out must not be _v.
D_MATH_FN void
d_mat_@S@_multiply_vector(
    @T@*       _out,
    const @T@* _a,
    const @T@* _v,
    size_t     _rows,
    size_t     _cols
)
{
    const bool in_range =
        ( (d_internal_vec_@S@_reach(_a, 1u, _rows * _cols) == 0) &&
          (d_internal_vec_@S@_reach(_v, 1u, _cols) == 0) );
    const @T@  zero     = (@T@)0;
    size_t     i        = 0u;

    // each entry, row i of _a by _v
    for (i = 0u; i < _rows; ++i)
    {
        const @T@* row = _a + (i * _cols);

        // in range: no test of its own
        if (in_range)
        {
            _out[i] = d_internal_vec_@S@_dot2_direct(false, row, 1u, _v, 1u,
                                                     _cols);
        }
        else
        {
            _out[i] = d_internal_vec_@S@_dot2(zero, false, row, 1u, _v, 1u, _v,
                                              1u, false, _cols);
        }
    }

    return;
}

// d_mat_@S@_multiply_vector_fast
//   _out (_rows) = _a (_rows x _cols) * _v (_cols) as written
// (d_vec_@S@_dot_fast for each row).
D_MATH_FN void
d_mat_@S@_multiply_vector_fast(
    @T@*       _out,
    const @T@* _a,
    const @T@* _v,
    size_t     _rows,
    size_t     _cols
)
{
    size_t i = 0u;

    for (i = 0u; i < _rows; ++i)
    {
        _out[i] = d_vec_@S@_dot_fast(_a + (i * _cols), _v, _cols);
    }

    return;
}

// d_internal_mat_@S@_power
//   _out = _a^_exponent for an _n x _n matrix, the identity for 0: by
// squaring, from the exponent's highest bit down, the power begun as _a and,
// at each bit below it, squared, then multiplied by _a on the left where the
// bit is 1, as A^(2k) = A^k A^k and A^(2k+1) = A A^(2k) compose: at most
// 2 log2(_exponent) products, where A^k = A * A^(k-1) takes _exponent - 1.
// _scratch holds _n * _n entries; neither it nor _out may be _a. Each
// product is d_mat_@S@_multiply's, compensated, or where _fast
// d_mat_@S@_multiply_fast's, as written.
D_MATH_FN void
d_internal_mat_@S@_power(
    @T@*       _out,
    @T@*       _scratch,
    const @T@* _a,
    size_t     _n,
    size_t     _exponent,
    bool       _fast
)
{
    size_t top = 1u;
    size_t bit = 0u;
    size_t i   = 0u;

    // the identity, for 0
    if (_exponent == 0u)
    {
        d_mat_@S@_identity(_out, _n);

        return;
    }

    // the exponent's highest bit
    while (top <= (_exponent / 2u))
    {
        top *= 2u;
    }

    // the power begun as _a
    for (i = 0u; i < (_n * _n); ++i)
    {
        _out[i] = _a[i];
    }

    // each bit below: the power squared, then times _a where the bit is 1
    for (bit = top / 2u; bit > 0u; bit /= 2u)
    {
        if (_fast)
        {
            d_mat_@S@_multiply_fast(_scratch, _out, _out, _n, _n, _n);
        }
        else
        {
            d_mat_@S@_multiply(_scratch, _out, _out, _n, _n, _n);
        }

        for (i = 0u; i < (_n * _n); ++i)
        {
            _out[i] = _scratch[i];
        }

        if ((_exponent & bit) != 0u)
        {
            if (_fast)
            {
                d_mat_@S@_multiply_fast(_scratch, _a, _out, _n, _n, _n);
            }
            else
            {
                d_mat_@S@_multiply(_scratch, _a, _out, _n, _n, _n);
            }

            for (i = 0u; i < (_n * _n); ++i)
            {
                _out[i] = _scratch[i];
            }
        }
    }

    return;
}

// d_mat_@S@_power
//   _out = _a^_exponent by squaring (d_internal_mat_@S@_power), each product
// compensated (d_mat_@S@_multiply). _scratch holds _n * _n entries; neither
// it nor _out may be _a.
D_MATH_FN void
d_mat_@S@_power(
    @T@*       _out,
    @T@*       _scratch,
    const @T@* _a,
    size_t     _n,
    size_t     _exponent
)
{
    d_internal_mat_@S@_power(_out, _scratch, _a, _n, _exponent, false);

    return;
}

// d_mat_@S@_power_fast
//   _out = _a^_exponent by squaring, each product as written
// (d_mat_@S@_multiply_fast).
D_MATH_FN void
d_mat_@S@_power_fast(
    @T@*       _out,
    @T@*       _scratch,
    const @T@* _a,
    size_t     _n,
    size_t     _exponent
)
{
    d_internal_mat_@S@_power(_out, _scratch, _a, _n, _exponent, true);

    return;
}

// @SECTION@.4    Square-matrix queries
//------------------------------------------------------------------------------
// trace sums the diagonal, rounded once from Sum2's pair (S404), and
// trace_fast as written; is_symmetric and is_identity allow each entry
// _tolerance.

// d_mat_@S@_trace
//   the sum of the _n x _n matrix's diagonal, rounded once from Sum2's pair;
// where an entry of it is a NaN or an infinity, d_mat_@S@_trace_fast's.
D_MATH_FN @T@
d_mat_@S@_trace(
    const @T@* _a,
    size_t     _n
)
{
    return d_internal_vec_@S@_sum2(_a, _n + 1u, _n);
}

// d_mat_@S@_trace_fast
//   the sum of the diagonal as written, in order from +0.
D_MATH_FN @T@
d_mat_@S@_trace_fast(
    const @T@* _a,
    size_t     _n
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _n; ++i)
    {
        acc += _a[(i * _n) + i];
    }

    return acc;
}

D_MATH_FN bool
d_mat_@S@_is_symmetric(
    const @T@* _a,
    size_t     _n,
    @T@        _tolerance
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = i + 1u;

        for (j = i + 1u; j < _n; ++j)
        {
            const @T@ d = _a[(i * _n) + j] - _a[(j * _n) + i];

            if (((d < (@T@)0) ? -d : d) > _tolerance)
            {
                return false;
            }
        }
    }

    return true;
}

D_MATH_FN bool
d_mat_@S@_is_identity(
    const @T@* _a,
    size_t     _n,
    @T@        _tolerance
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _n; ++j)
        {
            const @T@ d = _a[(i * _n) + j] - ((i == j) ? (@T@)1 : (@T@)0);

            if (((d < (@T@)0) ? -d : d) > _tolerance)
            {
                return false;
            }
        }
    }

    return true;
}

// @SECTION@.5    Determinant, inverse, solution and cofactors
//------------------------------------------------------------------------------
// The square matrix's algebra, as linear_algebra/square.hpp computes it: the
// determinant by Bareiss's fraction-free elimination (exact for integers,
// and the subframework's algorithm), the inverse by Gauss-Jordan with
// partial pivoting, and the solution of A X = B by Gaussian elimination of
// [A | B] with partial pivoting, LAPACK's dgesv (S257), then refined with
// extra precision, as LAPACK's dgerfsx refines (S406). Each product of the
// eliminations is rounded before it is combined; the refinement's residual
// is Dot2's (vec.h). Every kernel takes the scratch it needs from the
// caller.

// d_mat_@S@_determinant
//   the determinant of the _n x _n matrix _a; _scratch holds _n * _n
// entries. Bareiss: a row with a zero pivot is swapped for the first below
// it with a non-zero entry in the column, flipping the sign; none means 0.
D_MATH_FN @T@
d_mat_@S@_determinant(
    const @T@* _a,
    @T@*       _scratch,
    size_t     _n
)
{
    @T@    prev = (@T@)1;
    @T@    sign = (@T@)1;
    size_t k    = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    for (k = 0u; k < _n; ++k)
    {
        size_t i = 0u;

        // a zero pivot: swap in the first row below with a non-zero entry
        if (_scratch[(k * _n) + k] == (@T@)0)
        {
            size_t swap_row = k;
            size_t r        = k + 1u;
            size_t c        = 0u;

            for (r = k + 1u; r < _n; ++r)
            {
                if (_scratch[(r * _n) + k] != (@T@)0)
                {
                    swap_row = r;
                    break;
                }
            }

            // the column is zero below the diagonal: the matrix is singular
            if (swap_row == k)
            {
                return (@T@)0;
            }

            for (c = 0u; c < _n; ++c)
            {
                const @T@ t = _scratch[(k * _n) + c];

                _scratch[(k * _n) + c]        = _scratch[(swap_row * _n) + c];
                _scratch[(swap_row * _n) + c] = t;
            }

            sign = -sign;
        }

        // the fraction-free update of the block below and right
        for (i = k + 1u; i < _n; ++i)
        {
            size_t j = k + 1u;

            for (j = k + 1u; j < _n; ++j)
            {
                const @T@ keep  = _scratch[(k * _n) + k] *
                                  _scratch[(i * _n) + j];
                const @T@ cross = _scratch[(i * _n) + k] *
                                  _scratch[(k * _n) + j];
                // one operation per statement: an x87 evaluates a whole
                // expression at 64 bits and rounds once, at the end
                const @T@ diff  = keep - cross;

                _scratch[(i * _n) + j] = diff / prev;
            }
        }

        prev = _scratch[(k * _n) + k];
    }

    return sign * prev;
}

// d_mat_@S@_inverse
//   _out = the inverse of the _n x _n matrix _a, true; or, for a singular
// one, the zero matrix and false. _scratch holds _n * _n entries; neither it
// nor _out may be _a. Gauss-Jordan: the pivot is the first entry of largest
// magnitude at or below the diagonal.
D_MATH_FN bool
d_mat_@S@_inverse(
    @T@*       _out,
    @T@*       _scratch,
    const @T@* _a,
    size_t     _n
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _scratch[k] = _a[k];
    }

    d_mat_@S@_identity(_out, _n);

    for (col = 0u; col < _n; ++col)
    {
        size_t    pivot = col;
        @T@       maxv  = (_scratch[(col * _n) + col] < (@T@)0)
                              ? -_scratch[(col * _n) + col]
                              : _scratch[(col * _n) + col];
        size_t    r     = col + 1u;
        size_t    c     = 0u;
        @T@       d     = (@T@)0;

        // the first entry of largest magnitude at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = (_scratch[(r * _n) + col] < (@T@)0)
                              ? -_scratch[(r * _n) + col]
                              : _scratch[(r * _n) + col];

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (_scratch[(pivot * _n) + col] == (@T@)0)
        {
            d_vec_@S@_fill(_out, _n * _n, (@T@)0);

            return false;
        }

        // the pivot row up into place, in both
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const @T@ ta = _scratch[(col * _n) + c];
                const @T@ ti = _out[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = ta;
                _out[(col * _n) + c]       = _out[(pivot * _n) + c];
                _out[(pivot * _n) + c]     = ti;
            }
        }

        d = _scratch[(col * _n) + col];

        // the pivot row divided through
        for (c = 0u; c < _n; ++c)
        {
            _scratch[(col * _n) + c] = _scratch[(col * _n) + c] / d;
            _out[(col * _n) + c]     = _out[(col * _n) + c] / d;
        }

        // every other row cleared in the pivot's column
        for (r = 0u; r < _n; ++r)
        {
            if (r != col)
            {
                const @T@ f = _scratch[(r * _n) + col];

                for (c = 0u; c < _n; ++c)
                {
                    const @T@ fa = f * _scratch[(col * _n) + c];
                    const @T@ fi = f * _out[(col * _n) + c];

                    _scratch[(r * _n) + c] = _scratch[(r * _n) + c] - fa;
                    _out[(r * _n) + c]     = _out[(r * _n) + c] - fi;
                }
            }
        }
    }

    return true;
}

// d_mat_@S@_solve_fast
//   _x = the solution X of A X = B, A n by n and B n by _m, true; or, for a
// singular A, the zero matrix and false. Gaussian elimination of [A | B]
// with partial pivoting - the pivot the first entry of largest magnitude
// at or below the diagonal, the rows below less their multiples of it, B's
// rows along - then back-substitution, as LAPACK's dgesv (S257): for each
// column of B the same bits as decomp.h's d_decomp_@S@_lu and
// d_decomp_@S@_lu_solve give. _scratch holds n * n entries; neither it nor
// _x may be _a or _b.
D_MATH_FN bool
d_mat_@S@_solve_fast(
    @T@*       _x,
    @T@*       _scratch,
    const @T@* _a,
    const @T@* _b,
    size_t     _n,
    size_t     _m
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
        @T@    maxv  = (_scratch[(col * _n) + col] < (@T@)0)
                           ? -_scratch[(col * _n) + col]
                           : _scratch[(col * _n) + col];
        size_t r     = 0u;
        size_t c     = 0u;

        // the largest magnitude in the column, at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = (_scratch[(r * _n) + col] < (@T@)0)
                              ? -_scratch[(r * _n) + col]
                              : _scratch[(r * _n) + col];

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (_scratch[(pivot * _n) + col] == (@T@)0)
        {
            for (k = 0u; k < (_n * _m); ++k)
            {
                _x[k] = (@T@)0;
            }

            return false;
        }

        // the pivot row up, in A and in B
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const @T@ t = _scratch[(col * _n) + c];

                _scratch[(col * _n) + c]   = _scratch[(pivot * _n) + c];
                _scratch[(pivot * _n) + c] = t;
            }

            for (c = 0u; c < _m; ++c)
            {
                const @T@ t = _x[(col * _m) + c];

                _x[(col * _m) + c]   = _x[(pivot * _m) + c];
                _x[(pivot * _m) + c] = t;
            }
        }

        // the rows below less their multiples of it
        {
            const @T@ pivot_value = _scratch[(col * _n) + col];

            for (r = col + 1u; r < _n; ++r)
            {
                const @T@ f = _scratch[(r * _n) + col] / pivot_value;

                for (c = col + 1u; c < _n; ++c)
                {
                    const @T@ prod = f * _scratch[(col * _n) + c];

                    _scratch[(r * _n) + c] = _scratch[(r * _n) + c] - prod;
                }

                for (c = 0u; c < _m; ++c)
                {
                    const @T@ prod = f * _x[(col * _m) + c];

                    _x[(r * _m) + c] = _x[(r * _m) + c] - prod;
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
            @T@    s = _x[(i * _m) + k];
            size_t j = 0u;

            for (j = i + 1u; j < _n; ++j)
            {
                const @T@ prod = _scratch[(i * _n) + j] * _x[(j * _m) + k];

                s = s - prod;
            }

            _x[(i * _m) + k] = s / _scratch[(i * _n) + i];
        }
    }

    return true;
}

// d_internal_mat_@S@_lu
//   _lu = the LU factors of the _n x _n matrix _a with partial pivoting, as
// d_mat_@S@_solve_fast eliminates: U on and above the diagonal, each
// multiplier below it, in the row it was applied to, the rows interchanged
// as the pivots were taken, and _pivots[k] the row column k's pivot came
// from: true; false for a singular _a, a column with no pivot.
D_MATH_FN bool
d_internal_mat_@S@_lu(
    @T@*       _lu,
    size_t*    _pivots,
    const @T@* _a,
    size_t     _n
)
{
    size_t col = 0u;
    size_t k   = 0u;

    for (k = 0u; k < (_n * _n); ++k)
    {
        _lu[k] = _a[k];
    }

    // each column: its pivot, the interchange, the rows below less it
    for (col = 0u; col < _n; ++col)
    {
        size_t pivot = col;
        @T@    maxv  = (_lu[(col * _n) + col] < (@T@)0)
                           ? -_lu[(col * _n) + col]
                           : _lu[(col * _n) + col];
        size_t r     = 0u;
        size_t c     = 0u;

        // the largest magnitude in the column, at or below the diagonal
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ v = (_lu[(r * _n) + col] < (@T@)0)
                              ? -_lu[(r * _n) + col]
                              : _lu[(r * _n) + col];

            if (v > maxv)
            {
                maxv  = v;
                pivot = r;
            }
        }

        // no pivot: singular
        if (_lu[(pivot * _n) + col] == (@T@)0)
        {
            return false;
        }

        _pivots[col] = pivot;

        // the pivot row up, whole
        if (pivot != col)
        {
            for (c = 0u; c < _n; ++c)
            {
                const @T@ t = _lu[(col * _n) + c];

                _lu[(col * _n) + c]   = _lu[(pivot * _n) + c];
                _lu[(pivot * _n) + c] = t;
            }
        }

        // the rows below less their multiples of it, each multiplier kept
        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ f = _lu[(r * _n) + col] / _lu[(col * _n) + col];

            _lu[(r * _n) + col] = f;

            for (c = col + 1u; c < _n; ++c)
            {
                const @T@ prod = f * _lu[(col * _n) + c];

                _lu[(r * _n) + c] = _lu[(r * _n) + c] - prod;
            }
        }
    }

    return true;
}

// d_internal_mat_@S@_lu_solve
//   _y (_n entries, _stride apart) = the solution of A y = _y by A's factors
// (d_internal_mat_@S@_lu): the interchanges in order, then L's forward
// substitution and U's back-substitution, each product rounded before it is
// combined - d_mat_@S@_solve_fast's arithmetic on one column of B, its
// interchanges carried through the eliminations the same: bit for bit where
// the family is evaluated in its own type.
D_MATH_FN void
d_internal_mat_@S@_lu_solve(
    @T@*          _y,
    size_t        _stride,
    const @T@*    _lu,
    const size_t* _pivots,
    size_t        _n
)
{
    size_t col = 0u;
    size_t i   = 0u;

    // the interchanges, in the order they were made
    for (col = 0u; col < _n; ++col)
    {
        if (_pivots[col] != col)
        {
            const @T@ t = _y[col * _stride];

            _y[col * _stride]          = _y[_pivots[col] * _stride];
            _y[_pivots[col] * _stride] = t;
        }
    }

    // forward: each row below less its multiple of the row
    for (col = 0u; col < _n; ++col)
    {
        size_t r = 0u;

        for (r = col + 1u; r < _n; ++r)
        {
            const @T@ prod = _lu[(r * _n) + col] * _y[col * _stride];

            _y[r * _stride] = _y[r * _stride] - prod;
        }
    }

    // back-substitution, the last row first
    for (i = _n; i-- > 0u; )
    {
        @T@    s = _y[i * _stride];
        size_t j = 0u;

        for (j = i + 1u; j < _n; ++j)
        {
            const @T@ prod = _lu[(i * _n) + j] * _y[j * _stride];

            s = s - prod;
        }

        _y[i * _stride] = s / _lu[(i * _n) + i];
    }

    return;
}

// d_internal_mat_@S@_correction
//   _fix = the correction of the solution _x + _tail (_x's entries _sx apart)
// of A x = b, b's entries _sb apart: the residual b - A (x + tail), each row
// by Dot2 rounded once (vec.h's d_internal_vec_@S@_dot2), solved by A's
// factors. *_largest is its largest magnitude, *_relative its largest
// relative to its entry of x, at most 1 - 1 also where that entry is 0.
D_MATH_FN void
d_internal_mat_@S@_correction(
    @T@*          _fix,
    @T@*          _largest,
    @T@*          _relative,
    const @T@*    _x,
    size_t        _sx,
    const @T@*    _tail,
    const @T@*    _a,
    const @T@*    _b,
    size_t        _sb,
    const @T@*    _lu,
    const size_t* _pivots,
    size_t        _n
)
{
    size_t i = 0u;

    *_largest  = (@T@)0;
    *_relative = (@T@)0;

    // the residual, row by row
    for (i = 0u; i < _n; ++i)
    {
        _fix[i] = d_internal_vec_@S@_dot2(_b[i * _sb], true, _a + (i * _n), 1u,
                                          _x, _sx, _tail, 1u, true, _n);
    }

    d_internal_mat_@S@_lu_solve(_fix, 1u, _lu, _pivots, _n);

    // its sizes, whole and against x
    for (i = 0u; i < _n; ++i)
    {
        const @T@ xe = _x[i * _sx];
        const @T@ d  = (_fix[i] < (@T@)0) ? -_fix[i] : _fix[i];
        const @T@ x  = (xe < (@T@)0) ? -xe : xe;
        const @T@ r  = (d >= x) ? (@T@)1 : (d / x);

        if (d > *_largest)
        {
            *_largest = d;
        }

        // a zero correction leaves its entry as it is
        if ( (d != (@T@)0) &&
             (r > *_relative) )
        {
            *_relative = r;
        }
    }

    return;
}

// d_internal_mat_@S@_refine
//   _x, one column of the solution of A x = b (_n entries _sx apart, b's _sb
// apart) as A's factors gave it, refined (d_mat_@S@_solve); _tail and _fix
// are _n entries each.
D_MATH_FN void
d_internal_mat_@S@_refine(
    @T@*          _x,
    size_t        _sx,
    @T@*          _tail,
    @T@*          _fix,
    const @T@*    _a,
    const @T@*    _b,
    size_t        _sb,
    const @T@*    _lu,
    const size_t* _pivots,
    size_t        _n
)
{
    @T@    last_largest  = (@T@)0;
    @T@    last_relative = (@T@)0;
    size_t step          = 0u;
    size_t i             = 0u;

    for (i = 0u; i < _n; ++i)
    {
        _tail[i] = (@T@)0;
    }

    // each step: the correction, then whether it is worth adding
    for (step = 0u; step < (size_t)D_CFG_MATH_MAT_REFINE_STEPS; ++step)
    {
        @T@ largest  = (@T@)0;
        @T@ relative = (@T@)0;

        d_internal_mat_@S@_correction(_fix, &largest, &relative, _x, _sx, _tail,
                                      _a, _b, _sb, _lu, _pivots, _n);

        // no progress: neither the correction nor its relative size halved
        if ( (step > 0u) &&
             (largest > (last_largest / (@T@)2)) &&
             (relative > (last_relative / (@T@)2)) )
        {
            break;
        }

        // the correction into the solution and its tail
        for (i = 0u; i < _n; ++i)
        {
            d_internal_vec_@S@_add_pair(&_x[i * _sx], &_tail[i], _fix[i]);
        }

        // converged: every entry's correction below half its unit in the
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

// d_mat_@S@_solve
//   _x = the solution X of A X = B, A n by n and B n by _m, true; or, for a
// singular A, the zero matrix and false. Each column first as
// d_mat_@S@_solve_fast takes it, A's factors kept - the first n * n of
// _scratch's entries and the interchanges in _pivots - then refined as
// LAPACK's dgerfsx refines with extra precision (S406; Demmel, Hida, Kahan,
// Li, Mukherjee and Riedy, 2006): the residual b - A x by Dot2, x carried as
// a pair, x and its tail, the correction solved by the factors and added,
// until every entry's correction lies below half its unit in the last place
// (converged), neither the largest correction nor the largest relative to
// its entry halves (no progress, the last not added), or
// D_CFG_MATH_MAT_REFINE_STEPS steps have run. Each entry is then x and its
// tail rounded once: for an A far enough from singular - cond(A) times the
// family's epsilon well below 1 - the solution correctly rounded but where
// it lies within some cond(A) eps^2 of a midpoint. Where A or the column of
// B has a NaN or an infinity, or the first solution does, the column is
// solve_fast's. _scratch holds n * n + 2 n entries and _pivots n; neither
// may be an operand, nor _x _a or _b.
D_MATH_FN bool
d_mat_@S@_solve(
    @T@*       _x,
    @T@*       _scratch,
    size_t*    _pivots,
    const @T@* _a,
    const @T@* _b,
    size_t     _n,
    size_t     _m
)
{
    @T@* const tail = _scratch + (_n * _n);
    @T@* const fix  = tail + _n;
    const bool a_ok = (d_internal_vec_@S@_reach(_a, 1u, _n * _n) != 2);
    size_t     col  = 0u;
    size_t     k    = 0u;

    // singular: the zero matrix
    if (!d_internal_mat_@S@_lu(_scratch, _pivots, _a, _n))
    {
        for (k = 0u; k < (_n * _m); ++k)
        {
            _x[k] = (@T@)0;
        }

        return false;
    }

    // each column: solved by the factors, then refined
    for (col = 0u; col < _m; ++col)
    {
        for (k = 0u; k < _n; ++k)
        {
            _x[(k * _m) + col] = _b[(k * _m) + col];
        }

        d_internal_mat_@S@_lu_solve(_x + col, _m, _scratch, _pivots, _n);

        // finite throughout: refined
        if ( (a_ok) &&
             (d_internal_vec_@S@_reach(_b + col, _m, _n) != 2) &&
             (d_internal_vec_@S@_reach(_x + col, _m, _n) != 2) )
        {
            d_internal_mat_@S@_refine(_x + col, _m, tail, fix, _a, _b + col,
                                      _m, _scratch, _pivots, _n);
        }
    }

    return true;
}

// d_mat_@S@_submatrix
//   _out ((_n - 1) x (_n - 1)) = _a without row _row and column _col.
D_MATH_FN void
d_mat_@S@_submatrix(
    @T@*       _out,
    const @T@* _a,
    size_t     _n,
    size_t     _row,
    size_t     _col
)
{
    size_t rr = 0u;
    size_t r  = 0u;

    for (r = 0u; r < _n; ++r)
    {
        if (r != _row)
        {
            size_t cc = 0u;
            size_t c  = 0u;

            for (c = 0u; c < _n; ++c)
            {
                if (c != _col)
                {
                    _out[(rr * (_n - 1u)) + cc] = _a[(r * _n) + c];
                    ++cc;
                }
            }

            ++rr;
        }
    }

    return;
}

// d_mat_@S@_cofactor
//   (-1)^(_row + _col) times the minor, the determinant of the submatrix
// without _row and _col; _scratch holds 2 (_n - 1)^2 entries.
D_MATH_FN @T@
d_mat_@S@_cofactor(
    const @T@* _a,
    @T@*       _scratch,
    size_t     _n,
    size_t     _row,
    size_t     _col
)
{
    const size_t m    = _n - 1u;
    const @T@    sign = (((_row + _col) % 2u) == 0u) ? (@T@)1 : (@T@)-1;

    d_mat_@S@_submatrix(_scratch, _a, _n, _row, _col);

    return sign * d_mat_@S@_determinant(_scratch, _scratch + (m * m), m);
}

// d_mat_@S@_cofactor_matrix
//   _out (_n x _n) = every cofactor of _a, in place; _scratch holds
// 2 (_n - 1)^2 entries. _adjugate set writes the transpose instead: the
// adjugate.
D_MATH_FN void
d_mat_@S@_cofactor_matrix(
    @T@*       _out,
    const @T@* _a,
    @T@*       _scratch,
    size_t     _n,
    bool       _adjugate
)
{
    size_t i = 0u;

    for (i = 0u; i < _n; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < _n; ++j)
        {
            const @T@ c = d_mat_@S@_cofactor(_a, _scratch, _n, i, j);

            _out[_adjugate ? ((j * _n) + i) : ((i * _n) + j)] = c;
        }
    }

    return;
}

// @SECTION@.6    Rotations
//------------------------------------------------------------------------------
// The rotation matrices of linear_algebra/transform.hpp, from an angle's
// cosine _c and sine _s, which the face computes as its level allows.
// Counter-clockwise; one operation per statement.

// d_mat_@S@_rotation_2d
//   _out (2 x 2) = [c -s; s c].
D_MATH_FN void
d_mat_@S@_rotation_2d(
    @T@* _out,
    @T@  _c,
    @T@  _s
)
{
    _out[0] = _c;
    _out[1] = -_s;
    _out[2] = _s;
    _out[3] = _c;

    return;
}

// d_mat_@S@_rotation_3d
//   _out (3 x 3) = the rotation about axis _axis (0 x, 1 y, 2 z).
D_MATH_FN void
d_mat_@S@_rotation_3d(
    @T@*   _out,
    size_t _axis,
    @T@    _c,
    @T@    _s
)
{
    // the two axes the rotation turns, in right-handed order
    const size_t a = (_axis + 1u) % 3u;
    const size_t b = (_axis + 2u) % 3u;

    d_mat_@S@_identity(_out, 3u);

    _out[(a * 3u) + a] = _c;
    _out[(a * 3u) + b] = -_s;
    _out[(b * 3u) + a] = _s;
    _out[(b * 3u) + b] = _c;

    return;
}

// d_mat_@S@_rotation_axis
//   _out (3 x 3) = the rotation about the unit axis _k by the angle whose
// cosine and sine are _c and _s, Rodrigues's formula: c I + (1 - c) k k^T +
// s [k]x, each entry formed in the order c + (k_i k_j) t, (k_i k_j) t -+
// k_l s.
D_MATH_FN void
d_mat_@S@_rotation_axis(
    @T@*       _out,
    const @T@* _k,
    @T@        _c,
    @T@        _s
)
{
    const @T@ t = (@T@)1 - _c;
    size_t    i = 0u;

    for (i = 0u; i < 3u; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < 3u; ++j)
        {
            const @T@ kk    = _k[i] * _k[j];
            const @T@ along = kk * t;

            // the diagonal: c + k_i^2 t
            if (i == j)
            {
                _out[(i * 3u) + j] = _c + along;
            }
            // off the diagonal: k_i k_j t -+ k_l s, l the third axis
            else
            {
                const size_t l    = 3u - i - j;
                const @T@    turn = _k[l] * _s;
                // the cross term's sign: +s where (i, j, l) is an even
                // permutation's (i, j) = (1, 0), (0, 2), (2, 1)
                const bool   plus = ( ((i == 1u) && (j == 0u)) ||
                                      ((i == 0u) && (j == 2u)) ||
                                      ((i == 2u) && (j == 1u)) );

                _out[(i * 3u) + j] = plus ? (along + turn) : (along - turn);
            }
        }
    }

    return;
}
"""

MAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Construction
    2.  Rows, columns and the transpose
    3.  Products
    4.  Square-matrix queries
    5.  Determinant, inverse, solution and cofactors
    6.  Rotations
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Construction
    2.  Rows, columns and the transpose
    3.  Products
    4.  Square-matrix queries
    5.  Determinant, inverse, solution and cofactors
    6.  Rotations
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Construction
    2.  Rows, columns and the transpose
    3.  Products
    4.  Square-matrix queries
    5.  Determinant, inverse, solution and cofactors
    6.  Rotations
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_MAT_H
#define DJINTERP_C_MATH_MAT_H 1

// std
#include <stddef.h>          // size_t
// djinterp
#include "../../../config/c/modules/math/cfg_mat.h"  // D_CFG_MATH_MAT_*
#include "./math_common.h"   // D_MATH_FN
#include "./vec.h"           // d_vec_*_fill, _dot, the compensated sums
"""

MAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_MAT_H
"""

_MA = ("const @T@*", "_a")
_MO = ("@T@*", "_out")
_SZ = lambda n: ("size_t", n)
MAT_OVERLOADS = [
    ("D_MATH_FN", "void", "d_mat_identity", [_MO, _SZ("_n")]),
    ("D_MATH_FN", "void", "d_mat_diagonal", [_MO, ("const @T@*", "_diagonal"), _SZ("_n")]),
    ("D_MATH_FN", "void", "d_mat_row", [_MO, _MA, _SZ("_i"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_col", [_MO, _MA, _SZ("_j"), _SZ("_rows"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_transpose", [_MO, _MA, _SZ("_rows"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_multiply", [_MO, _MA, ("const @T@*", "_b"), _SZ("_rows"), _SZ("_inner"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_multiply_fast", [_MO, _MA, ("const @T@*", "_b"), _SZ("_rows"), _SZ("_inner"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_multiply_vector", [_MO, _MA, ("const @T@*", "_v"), _SZ("_rows"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_multiply_vector_fast", [_MO, _MA, ("const @T@*", "_v"), _SZ("_rows"), _SZ("_cols")]),
    ("D_MATH_FN", "void", "d_mat_power", [_MO, ("@T@*", "_scratch"), _MA, _SZ("_n"), _SZ("_exponent")]),
    ("D_MATH_FN", "void", "d_mat_power_fast", [_MO, ("@T@*", "_scratch"), _MA, _SZ("_n"), _SZ("_exponent")]),
    ("D_MATH_FN", "@T@", "d_mat_trace", [_MA, _SZ("_n")]),
    ("D_MATH_FN", "@T@", "d_mat_trace_fast", [_MA, _SZ("_n")]),
    ("D_MATH_FN", "bool", "d_mat_is_symmetric", [_MA, _SZ("_n"), ("@T@", "_tolerance")]),
    ("D_MATH_FN", "bool", "d_mat_is_identity", [_MA, _SZ("_n"), ("@T@", "_tolerance")]),
    ("D_MATH_FN", "@T@", "d_mat_determinant", [_MA, ("@T@*", "_scratch"), _SZ("_n")]),
    ("D_MATH_FN", "bool", "d_mat_inverse", [_MO, ("@T@*", "_scratch"), _MA, _SZ("_n")]),
    ("D_MATH_FN", "bool", "d_mat_solve_fast", [("@T@*", "_x"), ("@T@*", "_scratch"), _MA, ("const @T@*", "_b"), _SZ("_n"), _SZ("_m")]),
    ("D_MATH_FN", "bool", "d_mat_solve", [("@T@*", "_x"), ("@T@*", "_scratch"), ("size_t*", "_pivots"), _MA, ("const @T@*", "_b"), _SZ("_n"), _SZ("_m")]),
    ("D_MATH_FN", "void", "d_mat_submatrix", [_MO, _MA, _SZ("_n"), _SZ("_row"), _SZ("_col")]),
    ("D_MATH_FN", "@T@", "d_mat_cofactor", [_MA, ("@T@*", "_scratch"), _SZ("_n"), _SZ("_row"), _SZ("_col")]),
    ("D_MATH_FN", "void", "d_mat_cofactor_matrix", [_MO, _MA, ("@T@*", "_scratch"), _SZ("_n"), ("bool", "_adjugate")]),
    ("D_MATH_FN", "void", "d_mat_rotation_2d", [_MO, ("@T@", "_c"), ("@T@", "_s")]),
    ("D_MATH_FN", "void", "d_mat_rotation_3d", [_MO, _SZ("_axis"), ("@T@", "_c"), ("@T@", "_s")]),
    ("D_MATH_FN", "void", "d_mat_rotation_axis", [_MO, ("const @T@*", "_k"), ("@T@", "_c"), ("@T@", "_s")]),
]


def mat():
    lines = [
        "Fixed-size matrices: the C core of math/linear_algebra/matrix.hpp, for",
        "float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A matrix here is a caller's row-major array of rows * cols entries, as",
        "BLAS takes them; the C++ face's storage is that array. What is entry-wise",
        "- sums, scaling, norms, comparison - is vec.h's over the entries; this",
        "header has what knows the shape. Every kernel is a constant expression",
        "from C++14. The products and the trace are compensated, and solve",
        "refines its solution with extra precision; their _fast forms are the",
        "formulas as written (S404, S406).",
    ]
    text = banner("mat.h", lines, revised_on="2026.11.21")
    text += "\n" + MAT_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = MAT_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", MAT_OVERLOADS)
    text += MAT_TAIL
    return text
