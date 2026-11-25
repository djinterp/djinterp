"""gen_c_math.quat - the text of quat.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section,
                     textwrap)


# -----------------------------------------------------------------------------
# quat.h
# -----------------------------------------------------------------------------

QUAT_KERNELS_PURE = r"""

//==============================================================================
// 1.  THE KERNELS
//==============================================================================
// Once, in long double, shared by the three families (math_common.h's
// D_MATH_FN_KERNEL). A family's values are long doubles exactly, and its part
// is the kernel's rounded once (d_internal_math_*_round_pair). Sums that
// cancel are carried as pairs (elementary.h's section 1.1), each product of
// two parts exact by Dekker's, so that a part's error is some 2^-2P of its
// terms, P long double's precision - past twice a double's on the x87, and
// twice a double's where long double is double. Operands are scaled by powers
// of 2 to a largest part in [1, 2) where Dekker's products could overflow or
// lose their low parts.


// 1.1    Pairs and scaling
//------------------------------------------------------------------------------

// d_internal_quat_ld_finite
//   whether each of _v's _n parts is finite.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_finite(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // a NaN or an infinity among the parts
    for (i = 0u; i < _n; ++i)
    {
        if ( (_v[i] != _v[i]) ||
             (d_internal_cplx_ld_is_inf(_v[i])) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_ld_has_nan
//   whether any of _v's _n parts is a NaN.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_has_nan(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // a NaN among the parts
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != _v[i])
        {
            return true;
        }
    }

    return false;
}

// d_internal_quat_ld_has_inf
//   whether any of _v's _n parts is an infinity.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_has_inf(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // an infinity among the parts
    for (i = 0u; i < _n; ++i)
    {
        if (d_internal_cplx_ld_is_inf(_v[i]))
        {
            return true;
        }
    }

    return false;
}

// d_internal_quat_ld_is_zero
//   whether each of _v's _n parts is 0, of either sign.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_is_zero(
    const long double* _v,
    size_t             _n
)
{
    size_t i = 0u;

    // a part that is not 0
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != 0.0L)
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_ld_safe
//   whether each of _v's _n parts lies where Dekker's products of two such
// parts neither overflow nor lose their low parts: 0, or from 2^(EMIN/2 + P)
// to 2^(EMAX/2 - P) in magnitude, as cplx.h's d_internal_cplx_ld_safe.
D_MATH_FN_KERNEL bool
d_internal_quat_ld_safe(
    const long double* _v,
    size_t             _n
)
{
    const long double low  =
        d_internal_math_ld_pow2((LDBL_MIN_EXP / 2) + LDBL_MANT_DIG);
    const long double high =
        d_internal_math_ld_pow2((LDBL_MAX_EXP / 2) - LDBL_MANT_DIG);
    size_t            i    = 0u;

    // a part outside the range
    for (i = 0u; i < _n; ++i)
    {
        const long double a = d_internal_cplx_ld_abs(_v[i]);

        if ( (a != 0.0L) &&
             ( (a < low) ||
               (a > high) ) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_ld_exponent
//   the binade of the largest magnitude among _v's _n parts, each finite and
// one not 0.
D_MATH_FN_KERNEL int
d_internal_quat_ld_exponent(
    const long double* _v,
    size_t             _n
)
{
    long double largest = 0.0L;
    size_t      i       = 0u;

    // the largest magnitude
    for (i = 0u; i < _n; ++i)
    {
        const long double a = d_internal_cplx_ld_abs(_v[i]);

        if (a > largest)
        {
            largest = a;
        }
    }

    return d_internal_cplx_ld_exponent(largest);
}

// d_internal_quat_ld_scale_all
//   _out = each of _v's _n parts times 2^_k, each rounded once: exact but
// where it falls into the subnormals, 2^-2P and more below the largest part
// once that is in [1, 2). _out may be _v.
D_MATH_FN_KERNEL void
d_internal_quat_ld_scale_all(
    long double*       _out,
    const long double* _v,
    size_t             _n,
    int                _k
)
{
    size_t i = 0u;

    // each part by the power
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_cplx_ld_scale(_v[i], _k);
    }

    return;
}

// d_internal_quat_ld_scale_pair
//   _p 2^_k, each part scaled: exact but past the largest finite value or in
// the subnormals.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_scale_pair(
    struct dj_internal_math_ld_pair _p,
    int                             _k
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = d_internal_cplx_ld_scale(_p.high, _k);
    result.low  = d_internal_cplx_ld_scale(_p.low, _k);

    return result;
}

// d_internal_quat_ld_fill_nan
//   _out = _n pairs, each a NaN.
D_MATH_FN_KERNEL void
d_internal_quat_ld_fill_nan(
    struct dj_internal_math_ld_pair* _out,
    size_t                           _n
)
{
    size_t i = 0u;

    // every part a NaN
    for (i = 0u; i < _n; ++i)
    {
        _out[i].high = d_internal_math_ld_nan();
        _out[i].low  = 0.0L;
    }

    return;
}

// d_internal_quat_ld_direction
//   _out = the direction of _v's _n parts, at least one infinite: each
// infinite part +-1 / sqrt(their count), the rest 0 of their own signs - the
// limit of v / |v| as the finite parts are left behind.
D_MATH_FN_KERNEL void
d_internal_quat_ld_direction(
    long double*       _out,
    const long double* _v,
    size_t             _n
)
{
    size_t      count = 0u;
    size_t      i     = 0u;
    long double unit  = 1.0L;

    // the infinite parts' count
    for (i = 0u; i < _n; ++i)
    {
        if (d_internal_cplx_ld_is_inf(_v[i]))
        {
            ++count;
        }
    }

    // 1 / sqrt(count), for a count of 2, 3 or 4
    if (count == 2u)
    {
        unit = 0.707106781186547524400844362104849039L;
    }
    else if (count == 3u)
    {
        unit = 0.577350269189625764509148780501957456L;
    }
    else if (count == 4u)
    {
        unit = 0.5L;
    }

    // each part along the infinities
    for (i = 0u; i < _n; ++i)
    {
        const bool infinite = d_internal_cplx_ld_is_inf(_v[i]);

        _out[i] = d_internal_cplx_ld_copysign(infinite ? unit : 0.0L, _v[i]);
    }

    return;
}

// d_internal_quat_ld_along
//   _u _s, where _u is 0 a zero, its sign _u's times _s's: a part of a vector
// that is 0 stays 0, whatever the value it is taken along - an infinity or a
// NaN among them - as an exact 0 times anything is 0.
D_MATH_FN_KERNEL long double
d_internal_quat_ld_along(
    long double _u,
    long double _s
)
{
    // a zero part: a zero, of the product's sign
    if (_u == 0.0L)
    {
        const long double zero = d_internal_cplx_ld_copysign(0.0L, _s);

        return (d_internal_cplx_ld_copysign(1.0L, _u) < 0.0L) ? -zero : zero;
    }

    return _u * _s;
}

// d_internal_quat_ld_dot
//   the sum of the _n products _a[i] _b[i], in order, as a pair: each product
// exact (Dekker's, its factors safe or scaled), the high parts summed by
// two-sums, the low parts and the two-sums' errors after, and the pair
// renormalized - Ogita, Rump and Oishi's Dot2, as accurate as the sum
// computed in twice the precision and rounded once, so that its high part is
// the sum rounded once wherever it does not cancel past 2^P of its terms. A
// sum of 0 is +0, as IEEE's arithmetic makes an exact sum of terms not all
// 0; where every product is 0, their sum in order, -0 where each is -0.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_dot(
    const long double* _a,
    const long double* _b,
    size_t             _n
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };
    long double                     high   = 0.0L;
    long double                     rest   = 0.0L;
    long double                     zeros  = 0.0L;
    bool                            zero   = true;
    size_t                          i      = 0u;

    // each product exactly, its high part into the running sum
    for (i = 0u; i < _n; ++i)
    {
        const struct dj_internal_math_ld_pair p =
            d_internal_math_ld_product_exact(_a[i], _b[i]);
        const struct dj_internal_math_ld_pair s =
            d_internal_math_ld_two_sum(high, p.high);
        const long double term = _a[i] * _b[i];

        high  = s.high;
        rest  = rest + (s.low + p.low);
        zero  = ( (zero) &&
                  (term == 0.0L) );
        zeros = (i == 0u) ? term : (zeros + term);
    }

    result = d_internal_math_ld_two_sum(high, rest);

    // an exact zero: +0, or the zero products' own sum
    if (result.high == 0.0L)
    {
        result.high = zero ? zeros : 0.0L;
        result.low  = 0.0L;
    }

    return result;
}

// d_internal_quat_ld_over
//   (_u.high + _u.low) / (_n.high + _n.low) as a pair, _n.high in [1, 32):
// cplx.h's d_internal_cplx_ld_over with its last sum kept as a pair - the
// quotient q of the high parts, the residual u - q n formed exactly by
// Dekker's product, and q with the residual over n - the quotient to some
// 2^-2P of it. A zero is q itself, of its sign; a quotient too near the
// subnormals for Dekker's product q less q n_lo / n.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_over(
    struct dj_internal_math_ld_pair _u,
    struct dj_internal_math_ld_pair _n
)
{
    const long double               q0     = _u.high / _n.high;
    const long double               least  =
        LDBL_MIN * d_internal_math_ld_pow2(LDBL_MANT_DIG + 4);
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = q0;

    // a zero: itself, its sign kept
    if (q0 == 0.0L)
    {
        return result;
    }

    // too near the subnormals for Dekker's product
    if (d_internal_cplx_ld_abs(q0) < least)
    {
        result.high = q0 - (q0 * (_n.low / _n.high));

        return result;
    }

    {
        const struct dj_internal_math_ld_pair t =
            d_internal_math_ld_product_exact(q0, _n.high);
        // u - q n: u's high part less t's is exact, the two within an ulp
        const long double head = (_u.high - t.high) - t.low;
        const long double r    = (head + _u.low) - (q0 * _n.low);

        result = d_internal_math_ld_two_sum(q0, r / _n.high);
    }

    return result;
}

// d_internal_quat_ld_lerp
//   (1 - _t) _a + _t _b as a pair: 1 - t exactly as a pair, each product
// a pair, their sum a pair - exact at t = 0 and t = 1.
D_MATH_FN_KERNEL struct dj_internal_math_ld_pair
d_internal_quat_ld_lerp(
    long double _a,
    long double _b,
    long double _t
)
{
    const struct dj_internal_math_ld_pair rest =
        d_internal_math_ld_two_sum(1.0L, -_t);
    const struct dj_internal_math_ld_pair from =
        d_internal_math_ld_pair_product(
            rest, d_internal_math_ld_pair_of(_a, 0.0L));
    const struct dj_internal_math_ld_pair to =
        d_internal_math_ld_product_exact(_t, _b);

    return d_internal_math_ld_pair_sum(from, to);
}

// d_internal_quat_ld_dot_sign
//   the sign of _a . _b, _n parts each, finite: their dot product's high part
// (d_internal_quat_ld_dot), each operand scaled first where it leaves the
// safe range; 0 where either is 0.
D_MATH_FN_KERNEL long double
d_internal_quat_ld_dot_sign(
    const long double* _a,
    const long double* _b,
    size_t             _n
)
{
    long double a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

    // a zero operand: no sign
    if ( (d_internal_quat_ld_is_zero(_a, _n)) ||
         (d_internal_quat_ld_is_zero(_b, _n)) )
    {
        return 0.0L;
    }

    d_internal_quat_ld_scale_all(a, _a, _n,
                                 -d_internal_quat_ld_exponent(_a, _n));
    d_internal_quat_ld_scale_all(b, _b, _n,
                                 -d_internal_quat_ld_exponent(_b, _n));

    return d_internal_quat_ld_dot(a, b, _n).high;
}


// 1.2    The algebra
//------------------------------------------------------------------------------
// Hamilton's product and the quotients, each part a sum of four products as
// d_internal_quat_ld_dot sums them, for finite operands; the families take
// the formulas as written where a part is infinite or a NaN.

// d_internal_quat_ld_hamilton
//   _out = _a _b, each part a pair (d_internal_quat_ld_dot), its four
// products in the order of the formula (d_quat_*_multiply_fast): x = w x' +
// x w' + y z' - z y', y = w y' - x z' + y w' + z x', z = w z' + x y' - y x' + z
// w', w = w w' - x x' - y y' - z z'. The factors safe, or scaled by the
// caller.
D_MATH_FN_KERNEL void
d_internal_quat_ld_hamilton(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b
)
{
    const long double x_a[4] = { _a[3],  _a[0],  _a[1], -_a[2] };
    const long double x_b[4] = { _b[0],  _b[3],  _b[2],  _b[1] };
    const long double y_a[4] = { _a[3], -_a[0],  _a[1],  _a[2] };
    const long double y_b[4] = { _b[1],  _b[2],  _b[3],  _b[0] };
    const long double z_a[4] = { _a[3],  _a[0], -_a[1],  _a[2] };
    const long double z_b[4] = { _b[2],  _b[1],  _b[0],  _b[3] };
    const long double w_a[4] = { _a[3], -_a[0], -_a[1], -_a[2] };
    const long double w_b[4] = { _b[3],  _b[0],  _b[1],  _b[2] };

    _out[0] = d_internal_quat_ld_dot(x_a, x_b, 4u);
    _out[1] = d_internal_quat_ld_dot(y_a, y_b, 4u);
    _out[2] = d_internal_quat_ld_dot(z_a, z_b, 4u);
    _out[3] = d_internal_quat_ld_dot(w_a, w_b, 4u);

    return;
}

// d_internal_quat_ld_multiply
//   _out = _a _b, both finite (d_internal_quat_ld_hamilton): where either
// leaves the safe range, each scaled to a largest part in [1, 2) and the
// parts scaled back, each rounded once more where it falls into the
// subnormals.
D_MATH_FN_KERNEL void
d_internal_quat_ld_multiply(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b
)
{
    // safe, or a zero operand: the parts as they are
    if ( ( (d_internal_quat_ld_safe(_a, 4u)) &&
           (d_internal_quat_ld_safe(_b, 4u)) ) ||
         (d_internal_quat_ld_is_zero(_a, 4u)) ||
         (d_internal_quat_ld_is_zero(_b, 4u)) )
    {
        d_internal_quat_ld_hamilton(_out, _a, _b);

        return;
    }

    {
        const int   ea   = d_internal_quat_ld_exponent(_a, 4u);
        const int   eb   = d_internal_quat_ld_exponent(_b, 4u);
        long double a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        long double b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        size_t      i    = 0u;

        d_internal_quat_ld_scale_all(a, _a, 4u, -ea);
        d_internal_quat_ld_scale_all(b, _b, 4u, -eb);
        d_internal_quat_ld_hamilton(_out, a, b);

        // each part back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_ld_scale_pair(_out[i], ea + eb);
        }
    }

    return;
}

// d_internal_quat_ld_inverse
//   _out = 1 / _q = q* / |q|^2, q finite and not 0: q scaled to a largest
// part in [1, 2), so that |q|^2, the sum of its exact squares as a pair, lies
// in [1, 16); each part of q* over it (d_internal_quat_ld_over), and scaled
// back.
D_MATH_FN_KERNEL void
d_internal_quat_ld_inverse(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const int   e    = d_internal_quat_ld_exponent(_q, 4u);
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(q, _q, 4u, -e);

    {
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);

        // each part of the conjugate over the squared norm
        for (i = 0u; i < 4u; ++i)
        {
            const long double part = (i < 3u) ? -q[i] : q[i];

            _out[i] = d_internal_quat_ld_scale_pair(
                d_internal_quat_ld_over(
                    d_internal_math_ld_pair_of(part, 0.0L), n),
                -e);
        }
    }

    return;
}

// d_internal_quat_ld_divide
//   _out = _p / _q = p q* / |q|^2 where _right, the quotient on the right, and
// q^-1 p = q* p / |q|^2 otherwise, both finite and q not 0: each operand
// scaled to a largest part in [1, 2), p left as it is where it is 0; the
// numerator's parts as pairs (d_internal_quat_ld_hamilton), each over |q|^2's
// pair (d_internal_quat_ld_over), scaled back by p's exponent less q's.
D_MATH_FN_KERNEL void
d_internal_quat_ld_divide(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _p,
    const long double*               _q,
    bool                             _right
)
{
    const int   eq   = d_internal_quat_ld_exponent(_q, 4u);
    const int   ep   = (d_internal_quat_ld_is_zero(_p, 4u)) ?
                           0 : d_internal_quat_ld_exponent(_p, 4u);
    long double p[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double c[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(p, _p, 4u, -ep);
    d_internal_quat_ld_scale_all(q, _q, 4u, -eq);
    c[0] = -q[0];
    c[1] = -q[1];
    c[2] = -q[2];
    c[3] = q[3];

    {
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);
        struct dj_internal_math_ld_pair num[4] = { { 0.0L, 0.0L },
                                                   { 0.0L, 0.0L },
                                                   { 0.0L, 0.0L },
                                                   { 0.0L, 0.0L } };

        // the numerator in its order
        if (_right)
        {
            d_internal_quat_ld_hamilton(num, p, c);
        }
        else
        {
            d_internal_quat_ld_hamilton(num, c, p);
        }

        // each part over the squared norm, back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_ld_scale_pair(
                d_internal_quat_ld_over(num[i], n), ep - eq);
        }
    }

    return;
}


// 1.3    The algebra in double
//------------------------------------------------------------------------------
// The product, the inverse and the quotients again, in double's pairs, for
// float's and double's where double is evaluated in its own type, as SSE
// evaluates it, and long double is not: a float's products exact in double,
// a double's by Dekker's (cplx.h's), each sum Dot2's - as accurate as
// twice double's precision, some ten times faster than the x87's long
// double. Where double is evaluated wider, as on a 32-bit x87, the families
// take section 1.2's kernels.
#if !D_MATH_DBL_EVAL_WIDE

// d_internal_quat_d2_is_zero
//   whether each of _v's _n parts is 0, of either sign.
D_MATH_FN_KERNEL bool
d_internal_quat_d2_is_zero(
    const double* _v,
    size_t        _n
)
{
    size_t i = 0u;

    // a part that is not 0
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != 0.0)
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_d2_safe
//   whether each of _v's _n parts is 0 or lies from 2^(EMIN/2 + P) to 2^(EMAX/2
// - P) in double's terms, where Dekker's products neither overflow nor lose
// their low parts (cplx.h's d_internal_cplx_d_safe).
D_MATH_FN_KERNEL bool
d_internal_quat_d2_safe(
    const double* _v,
    size_t        _n
)
{
    const double low  = d_internal_math_d_pow2((DBL_MIN_EXP / 2) +
                                               DBL_MANT_DIG);
    const double high = d_internal_math_d_pow2((DBL_MAX_EXP / 2) -
                                               DBL_MANT_DIG);
    size_t       i    = 0u;

    // a part outside the range
    for (i = 0u; i < _n; ++i)
    {
        const double a = d_internal_cplx_d_abs(_v[i]);

        if ( (a != 0.0) &&
             ( (a < low) ||
               (a > high) ) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_d2_scaled
//   _out = _v's _n parts times 2^-e, e the binade of the largest, which is
// then in [1, 2); e itself returned. _v finite and not 0.
D_MATH_FN_KERNEL int
d_internal_quat_d2_scaled(
    double*       _out,
    const double* _v,
    size_t        _n
)
{
    double largest = 0.0;
    int    e       = 0;
    size_t i       = 0u;

    // the largest magnitude
    for (i = 0u; i < _n; ++i)
    {
        const double a = d_internal_cplx_d_abs(_v[i]);

        if (a > largest)
        {
            largest = a;
        }
    }

    e = d_internal_cplx_d_exponent(largest);

    // each part by the power
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_cplx_d_scale(_v[i], -e);
    }

    return e;
}

// d_internal_quat_d2_scale_pair
//   _p 2^_k, each part scaled.
D_MATH_FN_KERNEL struct dj_internal_math_d_pair
d_internal_quat_d2_scale_pair(
    struct dj_internal_math_d_pair _p,
    int                            _k
)
{
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };

    result.high = d_internal_cplx_d_scale(_p.high, _k);
    result.low  = d_internal_cplx_d_scale(_p.low, _k);

    return result;
}

// d_internal_quat_d2_dot
//   d_internal_quat_ld_dot in double: the sum of the _n products as a pair,
// each exact by Dekker's (cplx.h's d_internal_cplx_d_product), Dot2's sum;
// an exact zero +0, or the zero products' own sum.
D_MATH_FN_KERNEL struct dj_internal_math_d_pair
d_internal_quat_d2_dot(
    const double* _a,
    const double* _b,
    size_t        _n
)
{
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };
    double                         high   = 0.0;
    double                         rest   = 0.0;
    double                         zeros  = 0.0;
    bool                           zero   = true;
    size_t                         i      = 0u;

    // each product exactly, its high part into the running sum
    for (i = 0u; i < _n; ++i)
    {
        const struct dj_internal_math_d_pair p =
            d_internal_cplx_d_product(_a[i], _b[i]);
        const struct dj_internal_math_d_pair s =
            d_internal_cplx_d_two_sum(high, p.high);
        const double term = _a[i] * _b[i];

        high  = s.high;
        rest  = rest + (s.low + p.low);
        zero  = ( (zero) &&
                  (term == 0.0) );
        zeros = (i == 0u) ? term : (zeros + term);
    }

    result = d_internal_cplx_d_two_sum(high, rest);

    // an exact zero: +0, or the zero products' own sum
    if (result.high == 0.0)
    {
        result.high = zero ? zeros : 0.0;
        result.low  = 0.0;
    }

    return result;
}

// d_internal_quat_d2_hamilton
//   d_internal_quat_ld_hamilton in double.
D_MATH_FN_KERNEL void
d_internal_quat_d2_hamilton(
    struct dj_internal_math_d_pair* _out,
    const double*                   _a,
    const double*                   _b
)
{
    const double x_a[4] = { _a[3],  _a[0],  _a[1], -_a[2] };
    const double x_b[4] = { _b[0],  _b[3],  _b[2],  _b[1] };
    const double y_a[4] = { _a[3], -_a[0],  _a[1],  _a[2] };
    const double y_b[4] = { _b[1],  _b[2],  _b[3],  _b[0] };
    const double z_a[4] = { _a[3],  _a[0], -_a[1],  _a[2] };
    const double z_b[4] = { _b[2],  _b[1],  _b[0],  _b[3] };
    const double w_a[4] = { _a[3], -_a[0], -_a[1], -_a[2] };
    const double w_b[4] = { _b[3],  _b[0],  _b[1],  _b[2] };

    _out[0] = d_internal_quat_d2_dot(x_a, x_b, 4u);
    _out[1] = d_internal_quat_d2_dot(y_a, y_b, 4u);
    _out[2] = d_internal_quat_d2_dot(z_a, z_b, 4u);
    _out[3] = d_internal_quat_d2_dot(w_a, w_b, 4u);

    return;
}

// d_internal_quat_d2_multiply
//   _out = _a _b, both finite (d_internal_quat_d2_hamilton), each scaled to
// a largest part in [1, 2) where either leaves the safe range, the parts
// scaled back.
D_MATH_FN_KERNEL void
d_internal_quat_d2_multiply(
    struct dj_internal_math_d_pair* _out,
    const double*                   _a,
    const double*                   _b
)
{
    // safe, or a zero operand: the parts as they are
    if ( ( (d_internal_quat_d2_safe(_a, 4u)) &&
           (d_internal_quat_d2_safe(_b, 4u)) ) ||
         (d_internal_quat_d2_is_zero(_a, 4u)) ||
         (d_internal_quat_d2_is_zero(_b, 4u)) )
    {
        d_internal_quat_d2_hamilton(_out, _a, _b);

        return;
    }

    {
        double    a[4] = { 0.0, 0.0, 0.0, 0.0 };
        double    b[4] = { 0.0, 0.0, 0.0, 0.0 };
        const int ea   = d_internal_quat_d2_scaled(a, _a, 4u);
        const int eb   = d_internal_quat_d2_scaled(b, _b, 4u);
        size_t    i    = 0u;

        d_internal_quat_d2_hamilton(_out, a, b);

        // each part back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_d2_scale_pair(_out[i], ea + eb);
        }
    }

    return;
}

// d_internal_quat_d2_over
//   d_internal_quat_ld_over in double: (_u.high + _u.low) / (_n.high +
// _n.low) as a pair, _n.high in [1, 32).
D_MATH_FN_KERNEL struct dj_internal_math_d_pair
d_internal_quat_d2_over(
    struct dj_internal_math_d_pair _u,
    struct dj_internal_math_d_pair _n
)
{
    const double                   q0     = _u.high / _n.high;
    const double                   least  =
        DBL_MIN * d_internal_math_d_pow2(DBL_MANT_DIG + 4);
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };

    result.high = q0;

    // a zero: itself, its sign kept
    if (q0 == 0.0)
    {
        return result;
    }

    // too near the subnormals for Dekker's product
    if (d_internal_cplx_d_abs(q0) < least)
    {
        result.high = q0 - (q0 * (_n.low / _n.high));

        return result;
    }

    {
        const struct dj_internal_math_d_pair t =
            d_internal_cplx_d_product(q0, _n.high);
        // u - q n: u's high part less t's is exact, the two within an ulp
        const double head = (_u.high - t.high) - t.low;
        const double r    = (head + _u.low) - (q0 * _n.low);

        result = d_internal_cplx_d_two_sum(q0, r / _n.high);
    }

    return result;
}

// d_internal_quat_d2_divide
//   d_internal_quat_ld_divide in double, _q's inverse where _p is NULL: _p
// / _q where _right, q^-1 p otherwise, both finite and q not 0, each scaled
// to a largest part in [1, 2), the parts over |q|^2's pair, scaled back.
D_MATH_FN_KERNEL void
d_internal_quat_d2_divide(
    struct dj_internal_math_d_pair* _out,
    const double*                   _p,
    const double*                   _q,
    bool                            _right
)
{
    const double one[4] = { 0.0, 0.0, 0.0, 1.0 };
    double       p[4]   = { 0.0, 0.0, 0.0, 0.0 };
    double       q[4]   = { 0.0, 0.0, 0.0, 0.0 };
    double       c[4]   = { 0.0, 0.0, 0.0, 0.0 };
    const double* num_p = (_p == NULL) ? one : _p;
    const int    eq     = d_internal_quat_d2_scaled(q, _q, 4u);
    const int    ep     = (d_internal_quat_d2_is_zero(num_p, 4u)) ?
                              0 : d_internal_quat_d2_scaled(p, num_p, 4u);
    size_t       i      = 0u;

    c[0] = -q[0];
    c[1] = -q[1];
    c[2] = -q[2];
    c[3] = q[3];

    {
        const struct dj_internal_math_d_pair n =
            d_internal_quat_d2_dot(q, q, 4u);
        struct dj_internal_math_d_pair num[4] = { { 0.0, 0.0 },
                                                  { 0.0, 0.0 },
                                                  { 0.0, 0.0 },
                                                  { 0.0, 0.0 } };

        // the numerator in its order: the inverse's is q* itself
        if (_p == NULL)
        {
            for (i = 0u; i < 4u; ++i)
            {
                num[i].high = c[i];
            }
        }
        else if (_right)
        {
            d_internal_quat_d2_hamilton(num, p, c);
        }
        else
        {
            d_internal_quat_d2_hamilton(num, c, p);
        }

        // each part over the squared norm, back to its scale
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_d2_scale_pair(
                d_internal_quat_d2_over(num[i], n), ep - eq);
        }
    }

    return;
}

#endif  // !D_MATH_DBL_EVAL_WIDE


// 1.4    Rotation
//------------------------------------------------------------------------------
// q v q^-1 and its matrix, for any q not 0: q's norm cancels, so a q a few
// steps from unit length names the rotation of q / |q|, as glTF's, Eigen's
// and GLM's unit quaternions name theirs.

// d_internal_quat_ld_rotate
//   _out = q v q^-1, the rotation _q names applied to the 3-vector _v, both
// finite and not 0: ((w^2 - |u|^2) v + 2 (u . v) u + 2 w (u x v)) / |q|^2, q
// = (u, w). q and v are scaled to largest parts in [1, 2); the squares and
// products exact and their sums pairs, each part's three terms a pair, over
// |q|^2's pair (d_internal_quat_ld_over), v's scale restored.
D_MATH_FN_KERNEL void
d_internal_quat_ld_rotate(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q,
    const long double*               _v
)
{
    const int   eq   = d_internal_quat_ld_exponent(_q, 4u);
    const int   ev   = d_internal_quat_ld_exponent(_v, 3u);
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double v[3] = { 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(q, _q, 4u, -eq);
    d_internal_quat_ld_scale_all(v, _v, 3u, -ev);

    {
        // w^2 - |u|^2, |q|^2 and u . v
        const long double sq_a[4] = { q[3], -q[0], -q[1], -q[2] };
        const long double sq_b[4] = { q[3],  q[0],  q[1],  q[2] };
        const long double c0_a[2] = { q[1], -q[2] };
        const long double c0_b[2] = { v[2],  v[1] };
        const long double c1_a[2] = { q[2], -q[0] };
        const long double c1_b[2] = { v[0],  v[2] };
        const long double c2_a[2] = { q[0], -q[1] };
        const long double c2_b[2] = { v[1],   v[0] };
        const long double q_v[3]  = { q[0],   q[1],  q[2] };
        const struct dj_internal_math_ld_pair a =
            d_internal_quat_ld_dot(sq_a, sq_b, 4u);
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);
        const struct dj_internal_math_ld_pair b =
            d_internal_quat_ld_dot(q_v, v, 3u);
        // u x v
        const struct dj_internal_math_ld_pair c[3] = {
            d_internal_quat_ld_dot(c0_a, c0_b, 2u),
            d_internal_quat_ld_dot(c1_a, c1_b, 2u),
            d_internal_quat_ld_dot(c2_a, c2_b, 2u) };

        // each part's three terms, over |q|^2
        for (i = 0u; i < 3u; ++i)
        {
            const struct dj_internal_math_ld_pair t1 =
                d_internal_math_ld_pair_product(
                    a, d_internal_math_ld_pair_of(v[i], 0.0L));
            const struct dj_internal_math_ld_pair t2 =
                d_internal_math_ld_pair_product(
                    b, d_internal_math_ld_pair_of(2.0L * q[i], 0.0L));
            const struct dj_internal_math_ld_pair t3 =
                d_internal_math_ld_pair_product(
                    c[i], d_internal_math_ld_pair_of(2.0L * q[3], 0.0L));
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_sum(t1, t2), t3);

            _out[i] = d_internal_quat_ld_scale_pair(
                d_internal_quat_ld_over(s, n), ev);
        }
    }

    return;
}

// d_internal_quat_ld_to_matrix3
//   _out (3 x 3, by rows) = the rotation matrix of _q, finite and not 0: [w^2 +
// x^2 - y^2 - z^2, 2 (x y - w z), 2 (x z + w y); 2 (x y + w z), w^2 - x^2 +
// y^2 - z^2, 2 (y z - w x); 2 (x z - w y), 2 (y z + w x), w^2 - x^2 - y^2 +
// z^2] / |q|^2, each numerator a pair of exact products, over |q|^2's pair.
D_MATH_FN_KERNEL void
d_internal_quat_ld_to_matrix3(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const int   e    = d_internal_quat_ld_exponent(_q, 4u);
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_ld_scale_all(q, _q, 4u, -e);

    {
        const long double x  = q[0];
        const long double y  = q[1];
        const long double z  = q[2];
        const long double w  = q[3];
        const long double d0[4]   = { w,  x, -y, -z };
        const long double d1[4]   = { w, -x,  y, -z };
        const long double d2[4]   = { w, -x, -y,  z };
        const long double wxyz[4] = { w,  x,  y,  z };
        const long double xw_a[2] = { 2.0L * x, -2.0L * w };
        const long double xw_p[2] = { 2.0L * x,  2.0L * w };
        const long double yw_a[2] = { 2.0L * y, -2.0L * w };
        const long double yw_p[2] = { 2.0L * y,  2.0L * w };
        const long double yz[2]   = { y, z };
        const long double zy[2]   = { z, y };
        const long double zx[2]   = { z, x };
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_dot(q, q, 4u);
        const struct dj_internal_math_ld_pair num[9] = {
            d_internal_quat_ld_dot(d0, wxyz, 4u),
            d_internal_quat_ld_dot(xw_a, yz, 2u),
            d_internal_quat_ld_dot(xw_p, zy, 2u),
            d_internal_quat_ld_dot(xw_p, yz, 2u),
            d_internal_quat_ld_dot(d1, wxyz, 4u),
            d_internal_quat_ld_dot(yw_a, zx, 2u),
            d_internal_quat_ld_dot(xw_a, zy, 2u),
            d_internal_quat_ld_dot(yw_p, zx, 2u),
            d_internal_quat_ld_dot(d2, wxyz, 4u) };

        // each entry over |q|^2
        for (i = 0u; i < 9u; ++i)
        {
            _out[i] = d_internal_quat_ld_over(num[i], n);
        }
    }

    return;
}
"""


# the kernels that take a square root and nothing else: once with the core's
# _c root, a constant expression, once with the C library's; a correctly
# rounded root either way, so the two give the same bits.
QUAT_KERNELS_ROOT = r"""

// 1.@SUB@    @TITLE@
//------------------------------------------------------------------------------
// @PROSE@

// d_internal_quat_ld_root@KS@
//   the square root of the pair _s, _s.high normal and positive, as a pair:
// the root r of the high part, rounded once, and (s - r^2) / 2r, r^2 exact.
@KQ@ struct dj_internal_math_ld_pair
d_internal_quat_ld_root@KS@(
    struct dj_internal_math_ld_pair _s
)
{
    const long double                     r   = @SQRT@(_s.high);
    const struct dj_internal_math_ld_pair t   =
        d_internal_math_ld_product_exact(r, r);
    // s - r^2: s's high part less t's is exact, the two within an ulp
    const long double                    rest =
        ((_s.high - t.high) - t.low) + _s.low;

    return d_internal_math_ld_two_sum(r, rest / (2.0L * r));
}

// d_internal_quat_ld_norm@KS@
//   |_v|, _n parts, at most 4, as a pair: +inf where a part is infinite, a
// NaN part a NaN, +0 for 0; otherwise v scaled to a largest part in [1, 2),
// the sum of its exact squares as a pair (d_internal_quat_ld_dot), its root
// as a pair, scaled back - the norm to some 2^-2P of it, rounded once more
// where it falls into the subnormals.
@KQ@ struct dj_internal_math_ld_pair
d_internal_quat_ld_norm@KS@(
    const long double* _v,
    size_t             _n
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // an infinite part: an infinite norm, a NaN beside it or not
    if (d_internal_quat_ld_has_inf(_v, _n))
    {
        result.high = d_internal_math_ld_infinity();

        return result;
    }

    // a NaN part
    if (d_internal_quat_ld_has_nan(_v, _n))
    {
        result.high = d_internal_math_ld_nan();

        return result;
    }

    // a zero vector: +0
    if (d_internal_quat_ld_is_zero(_v, _n))
    {
        return result;
    }

    {
        const int   e    = d_internal_quat_ld_exponent(_v, _n);
        long double v[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

        d_internal_quat_ld_scale_all(v, _v, _n, -e);
        result = d_internal_quat_ld_scale_pair(
            d_internal_quat_ld_root@KS@(d_internal_quat_ld_dot(v, v, _n)), e);
    }

    return result;
}

// d_internal_quat_ld_normalize@KS@
//   _out = _q / |q|, each part a pair: q scaled to a largest part in [1, 2),
// |q| the root of its exact squares' sum as a pair, each part over it
// (d_internal_quat_ld_over). A zero q is left as it is; a NaN part makes
// each a NaN; an infinite part, none a NaN, the direction of the infinities
// (d_internal_quat_ld_direction), the limit of q / |q|.
@KQ@ void
d_internal_quat_ld_normalize@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    // a NaN part: no direction
    if (d_internal_quat_ld_has_nan(_q, 4u))
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    // an infinite part: the direction of the infinities
    if (d_internal_quat_ld_has_inf(_q, 4u))
    {
        d_internal_quat_ld_direction(q, _q, 4u);

        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(q[i], 0.0L);
        }

        return;
    }

    // zero: left as it is, signs of zero included
    if (d_internal_quat_ld_is_zero(_q, 4u))
    {
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(_q[i], 0.0L);
        }

        return;
    }

    d_internal_quat_ld_scale_all(q, _q, 4u,
                                 -d_internal_quat_ld_exponent(_q, 4u));

    {
        const struct dj_internal_math_ld_pair n =
            d_internal_quat_ld_root@KS@(d_internal_quat_ld_dot(q, q, 4u));

        // each part over the norm
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = d_internal_quat_ld_over(
                d_internal_math_ld_pair_of(q[i], 0.0L), n);
        }
    }

    return;
}

// d_internal_quat_ld_from_matrix3@KS@
//   _out = the unit quaternion of the rotation matrix _m (3 x 3, by rows), w
// >= 0, by Shepperd's method: of 4w^2 = 1 + m00 + m11 + m22, 4x^2 = 1 + m00 -
// m11 - m22, 4y^2 = 1 - m00 + m11 - m22 and 4z^2 = 1 - m00 - m11 + m22, each
// summed as a pair, the largest (w first among equals, then x, y and z) -
// at least 1, as the four sum to 4 - gives its part, its root over 2, and
// the others are the off-diagonal sums and differences over 4 times it; then
// all four negated where w is negative, so that q is the one of q and -q with
// w >= 0. A matrix a few steps from orthogonal gives a quaternion as many
// from unit length. A NaN or infinite entry: a NaN in each part.
@KQ@ void
d_internal_quat_ld_from_matrix3@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _m
)
{
    const long double one[4] = { 1.0L, 1.0L, 1.0L, 1.0L };
    const long double cx[4]  = { 1.0L,  _m[0], -_m[4], -_m[8] };
    const long double cy[4]  = { 1.0L, -_m[0],  _m[4], -_m[8] };
    const long double cz[4]  = { 1.0L, -_m[0], -_m[4],  _m[8] };
    const long double cw[4]  = { 1.0L,  _m[0],  _m[4],  _m[8] };
    size_t            k      = 3u;
    size_t            i      = 0u;

    // a NaN or an infinity: no rotation
    if (!d_internal_quat_ld_finite(_m, 9u))
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    {
        const struct dj_internal_math_ld_pair c[4] = {
            d_internal_quat_ld_dot(cx, one, 4u),
            d_internal_quat_ld_dot(cy, one, 4u),
            d_internal_quat_ld_dot(cz, one, 4u),
            d_internal_quat_ld_dot(cw, one, 4u) };

        // the largest candidate, w first among equals, then x, y, z
        for (i = 0u; i < 3u; ++i)
        {
            if (c[i].high > c[k].high)
            {
                k = i;
            }
        }

        {
            const struct dj_internal_math_ld_pair s =
                d_internal_quat_ld_root@KS@(c[k]);
            // 4 times the part: twice the root
            const struct dj_internal_math_ld_pair den =
                d_internal_math_ld_pair_of(2.0L * s.high, 2.0L * s.low);
            // the other three parts' numerators, in x, y, z, w order
            const long double m01 = _m[1];
            const long double m02 = _m[2];
            const long double m10 = _m[3];
            const long double m12 = _m[5];
            const long double m20 = _m[6];
            const long double m21 = _m[7];
            struct dj_internal_math_ld_pair num[4] = { { 0.0L, 0.0L },
                                                       { 0.0L, 0.0L },
                                                       { 0.0L, 0.0L },
                                                       { 0.0L, 0.0L } };

            // the off-diagonal sums and differences k's part divides
            if (k == 0u)
            {
                num[1] = d_internal_math_ld_two_sum(m01, m10);
                num[2] = d_internal_math_ld_two_sum(m02, m20);
                num[3] = d_internal_math_ld_two_sum(m21, -m12);
            }
            else if (k == 1u)
            {
                num[0] = d_internal_math_ld_two_sum(m01, m10);
                num[2] = d_internal_math_ld_two_sum(m12, m21);
                num[3] = d_internal_math_ld_two_sum(m02, -m20);
            }
            else if (k == 2u)
            {
                num[0] = d_internal_math_ld_two_sum(m02, m20);
                num[1] = d_internal_math_ld_two_sum(m12, m21);
                num[3] = d_internal_math_ld_two_sum(m10, -m01);
            }
            else
            {
                num[0] = d_internal_math_ld_two_sum(m21, -m12);
                num[1] = d_internal_math_ld_two_sum(m02, -m20);
                num[2] = d_internal_math_ld_two_sum(m10, -m01);
            }

            // k's part the root over 2, the others over 4 times it
            for (i = 0u; i < 4u; ++i)
            {
                _out[i] = (i == k) ?
                    d_internal_math_ld_pair_of(s.high / 2.0L, s.low / 2.0L) :
                    d_internal_quat_ld_over(num[i], den);
            }
        }
    }

    // q or -q: the one with w >= 0
    if (_out[3].high < 0.0L)
    {
        for (i = 0u; i < 4u; ++i)
        {
            _out[i].high = -_out[i].high;
            _out[i].low  = -_out[i].low;
        }
    }

    return;
}

// d_internal_quat_ld_from_two_vectors@KS@
//   _out = the unit quaternion of the shortest rotation turning the direction
// of _a into _b's, both 3-vectors: (a x b, |a| |b| + a . b) normalized, the
// sum's cancellation, where a . b < 0, avoided as |a x b|^2 / (|a| |b| - a .
// b), each term a pair of a and b scaled to largest parts in [1, 2). Where
// the two are exactly opposed, the half turn about the axis a x e_k, e_k the
// unit vector of a's smallest part (the first among equals). A zero vector:
// no rotation, the identity; a NaN or an infinite part: a NaN in each part.
@KQ@ void
d_internal_quat_ld_from_two_vectors@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b
)
{
    long double a[3] = { 0.0L, 0.0L, 0.0L };
    long double b[3] = { 0.0L, 0.0L, 0.0L };
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };

    // a NaN or an infinity: no rotation
    if ( (!d_internal_quat_ld_finite(_a, 3u)) ||
         (!d_internal_quat_ld_finite(_b, 3u)) )
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    // a zero vector names no direction: the identity
    if ( (d_internal_quat_ld_is_zero(_a, 3u)) ||
         (d_internal_quat_ld_is_zero(_b, 3u)) )
    {
        _out[0] = d_internal_math_ld_pair_of(0.0L, 0.0L);
        _out[1] = d_internal_math_ld_pair_of(0.0L, 0.0L);
        _out[2] = d_internal_math_ld_pair_of(0.0L, 0.0L);
        _out[3] = d_internal_math_ld_pair_of(1.0L, 0.0L);

        return;
    }

    d_internal_quat_ld_scale_all(a, _a, 3u,
                                 -d_internal_quat_ld_exponent(_a, 3u));
    d_internal_quat_ld_scale_all(b, _b, 3u,
                                 -d_internal_quat_ld_exponent(_b, 3u));

    {
        const long double c0_a[2] = { a[1], -a[2] };
        const long double c0_b[2] = { b[2],  b[1] };
        const long double c1_a[2] = { a[2], -a[0] };
        const long double c1_b[2] = { b[0],  b[2] };
        const long double c2_a[2] = { a[0], -a[1] };
        const long double c2_b[2] = { b[1],   b[0] };
        const struct dj_internal_math_ld_pair c[3] = {
            d_internal_quat_ld_dot(c0_a, c0_b, 2u),
            d_internal_quat_ld_dot(c1_a, c1_b, 2u),
            d_internal_quat_ld_dot(c2_a, c2_b, 2u) };
        const struct dj_internal_math_ld_pair d =
            d_internal_quat_ld_dot(a, b, 3u);
        // |a| |b|, the root of the product of the squared norms
        const struct dj_internal_math_ld_pair m =
            d_internal_quat_ld_root@KS@(d_internal_math_ld_pair_product(
                d_internal_quat_ld_dot(a, a, 3u),
                d_internal_quat_ld_dot(b, b, 3u)));
        struct dj_internal_math_ld_pair w = { 0.0L, 0.0L };

        // |a| |b| + a . b, as a sum where the two agree in sign
        if (d.high >= 0.0L)
        {
            w = d_internal_math_ld_pair_sum(m, d);
        }
        else
        {
            const struct dj_internal_math_ld_pair cc =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_sum(
                        d_internal_math_ld_pair_product(c[0], c[0]),
                        d_internal_math_ld_pair_product(c[1], c[1])),
                    d_internal_math_ld_pair_product(c[2], c[2]));
            const struct dj_internal_math_ld_pair den =
                d_internal_math_ld_pair_sum(
                    m, d_internal_math_ld_pair_of(-d.high, -d.low));

            // exactly opposed: |a x b| is 0
            if (cc.high != 0.0L)
            {
                w = d_internal_quat_ld_over(cc, den);
            }
        }

        q[0] = c[0].high;
        q[1] = c[1].high;
        q[2] = c[2].high;
        q[3] = w.high;
    }

    // exactly opposed: a half turn about an axis perpendicular to a
    if (d_internal_quat_ld_is_zero(q, 4u))
    {
        const long double x = d_internal_cplx_ld_abs(a[0]);
        const long double y = d_internal_cplx_ld_abs(a[1]);
        const long double z = d_internal_cplx_ld_abs(a[2]);

        // a x e_k, e_k along a's smallest part
        if ( (x <= y) &&
             (x <= z) )
        {
            q[1] = a[2];
            q[2] = -a[1];
        }
        else if (y <= z)
        {
            q[0] = -a[2];
            q[2] = a[0];
        }
        else
        {
            q[0] = a[1];
            q[1] = -a[0];
        }
    }

    d_internal_quat_ld_normalize@KS@(_out, q);

    return;
}

// d_internal_quat_ld_nlerp@KS@
//   _out = the normalized linear interpolation from _a (t = 0) to _b (t = 1),
// along the shorter arc - b negated where a . b < 0, as q and -q are one
// rotation: ((1 - t) a + t b) / |...|, the sum's parts as pairs of a and b
// scaled alike to a largest part in [1, 2) (d_internal_quat_ld_lerp), then
// normalized. A NaN or an infinite part, of t too: a NaN in each part.
@KQ@ void
d_internal_quat_ld_nlerp@KS@(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b,
    long double                      _t
)
{
    long double a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double l[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    // a NaN or an infinity: nothing to interpolate
    if ( (!d_internal_quat_ld_finite(_a, 4u)) ||
         (!d_internal_quat_ld_finite(_b, 4u)) ||
         (!d_internal_quat_ld_finite(&_t, 1u)) )
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    {
        const long double flip = (d_internal_quat_ld_dot_sign(_a, _b, 4u) <
                                  0.0L) ? -1.0L : 1.0L;
        const long double both[8] = { _a[0], _a[1], _a[2], _a[3],
                                      _b[0], _b[1], _b[2], _b[3] };
        const int         e       =
            (d_internal_quat_ld_is_zero(both, 8u)) ?
                0 : d_internal_quat_ld_exponent(both, 8u);

        d_internal_quat_ld_scale_all(a, _a, 4u, -e);
        d_internal_quat_ld_scale_all(b, _b, 4u, -e);

        // each part of the line, the shorter arc's end
        for (i = 0u; i < 4u; ++i)
        {
            l[i] = d_internal_quat_ld_lerp(a[i], flip * b[i], _t).high;
        }
    }

    d_internal_quat_ld_normalize@KS@(_out, l);

    return;
}
"""


QUAT_KERNELS_FUNCTIONS = r"""

// 1.7    The _c forms' functions
//------------------------------------------------------------------------------
// The exponential, logarithm, square root and real powers of a quaternion
// are the complex ones in its plane (S275): q = w + v, r = |v|, u = v / r,
// f(q) = Re f(w + r i) + u Im f(w + r i) - for a vector part along im alone
// the complex function itself (each family's section 6). r is carried as a
// pair, and f corrected to first order for its low part, as the plane's
// point is w + (r_hi + r_lo) i; the logarithm and the root take q scaled by
// a power of 2 where it leaves the safe range, so that nothing overflows or
// falls into the subnormals. Each is cplx.h's kernel, a constant expression
// from C++14.

// dj_internal_quat_ld_plane
//   a quaternion's plane: the point w + r i, r = |v| as a pair, v's direction
// u, and the power, 2^-scale, that w and r were taken at.
struct dj_internal_quat_ld_plane
{
    long double                     w;
    struct dj_internal_math_ld_pair r;
    long double                     u[3];
    int                             scale;
};

// d_internal_quat_ld_plane_c
//   _q's plane, its vector part not 0 and no part a NaN: where _scale is 1
// and q leaves the safe range (d_internal_quat_ld_safe), w and r taken at
// 2^-k, k the binade of q's largest part, and where it is 2 at the even one
// of k and k + 1; otherwise at 1. u is v over |v|, both as pairs of v
// scaled to a largest part in [1, 2), each part rounded once. A part of v
// infinite: r +inf and u the direction of v's infinities
// (d_internal_quat_ld_direction), q taken at 1.
D_MATH_FN_KERNEL struct dj_internal_quat_ld_plane
d_internal_quat_ld_plane_c(
    const long double* _q,
    int                _scale
)
{
    struct dj_internal_quat_ld_plane result = { 0.0L,
                                                { 0.0L, 0.0L },
                                                { 0.0L, 0.0L, 0.0L },
                                                0 };
    long double                      v[3]   = { _q[0], _q[1], _q[2] };
    size_t                           i      = 0u;

    result.w = _q[3];

    // an infinite part of v: an infinite r along v's infinities
    if (d_internal_quat_ld_has_inf(v, 3u))
    {
        result.r.high = d_internal_math_ld_infinity();
        d_internal_quat_ld_direction(result.u, v, 3u);

        return result;
    }

    // past the safe range: q's largest part into [1, 2), or [1/2, 2) at an
    // even power - only there, as log |q| near 0 would cancel against the
    // scale's k ln 2
    if ( (_scale > 0) &&
         (d_internal_quat_ld_finite(_q, 4u)) &&
         (!d_internal_quat_ld_safe(_q, 4u)) )
    {
        result.scale = d_internal_quat_ld_exponent(_q, 4u);

        if ( (_scale == 2) &&
             ((result.scale % 2) != 0) )
        {
            result.scale = result.scale + 1;
        }

        result.w = d_internal_cplx_ld_scale(_q[3], -result.scale);
    }

    {
        const int   e    = d_internal_quat_ld_exponent(v, 3u);
        long double s[3] = { 0.0L, 0.0L, 0.0L };

        d_internal_quat_ld_scale_all(s, v, 3u, -e);

        {
            const struct dj_internal_math_ld_pair root =
                d_internal_quat_ld_root_c(d_internal_quat_ld_dot(s, s, 3u));

            // u = v / |v|
            for (i = 0u; i < 3u; ++i)
            {
                result.u[i] = d_internal_quat_ld_over(
                    d_internal_math_ld_pair_of(s[i], 0.0L), root).high;
            }

            result.r = d_internal_quat_ld_scale_pair(root, e - result.scale);
        }
    }

    return result;
}

// d_internal_quat_ld_relative
//   (r_lo i) / z, z = _w + _r.high i: the plane point's low part relative to
// it, the first-order correction's factor, from w and r scaled alike to a
// larger one in [1, 2); 0 where r's low part is 0 or z is not finite.
D_MATH_FN_KERNEL struct dj_cplx_ld
d_internal_quat_ld_relative(
    long double                     _w,
    struct dj_internal_math_ld_pair _r
)
{
    // nothing to correct
    if ( (_r.low == 0.0L) ||
         (_w != _w) ||
         (d_internal_cplx_ld_is_inf(_w)) ||
         (d_internal_cplx_ld_is_inf(_r.high)) )
    {
        return d_internal_cplx_ld_make(0.0L, 0.0L);
    }

    {
        const long double big   = (d_internal_cplx_ld_abs(_w) > _r.high) ?
                                      d_internal_cplx_ld_abs(_w) : _r.high;
        const int         k     = d_internal_cplx_ld_exponent(big);
        const long double a     = d_internal_cplx_ld_scale(_r.high, -k);
        const long double b     = d_internal_cplx_ld_scale(_w, -k);
        const long double m     = (a * a) + (b * b);
        const long double ratio = _r.low / _r.high;
        // i ratio a / (b + a i) = ratio a (a + b i) / m
        const long double f     = (ratio * a) / m;

        return d_internal_cplx_ld_make(f * a, f * b);
    }
}

// d_internal_quat_ld_corrected
//   _f + _g _rho, a complex value plus its first-order correction: _f
// itself where either is not finite or _rho is 0.
D_MATH_FN_KERNEL struct dj_cplx_ld
d_internal_quat_ld_corrected(
    struct dj_cplx_ld _f,
    struct dj_cplx_ld _g,
    struct dj_cplx_ld _rho
)
{
    // nothing to add
    if ( ( (_rho.part[0] == 0.0L) &&
           (_rho.part[1] == 0.0L) ) ||
         (!d_internal_cplx_ld_finite(_f)) ||
         (!d_internal_cplx_ld_finite(_g)) )
    {
        return _f;
    }

    {
        const long double re = (_g.part[0] * _rho.part[0]) -
                               (_g.part[1] * _rho.part[1]);
        const long double im = (_g.part[0] * _rho.part[1]) +
                               (_g.part[1] * _rho.part[0]);

        return d_internal_cplx_ld_make(_f.part[0] + re, _f.part[1] + im);
    }
}

// d_internal_quat_ld_exp_c
//   _out = e^q = e^w (cos r + u sin r), _q's vector part not along im alone
// and no part a NaN: cplx.h's exponential at w + r_hi i, plus e^z r_lo i, its
// imaginary part along u (d_internal_quat_ld_along).
D_MATH_FN_KERNEL void
d_internal_quat_ld_exp_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 0);
    const struct dj_cplx_ld f = d_internal_cplx_ld_exp_c(
        d_internal_cplx_ld_make(p.w, p.r.high));
    // d e^z = e^z dz, dz = r_lo i
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, f, d_internal_cplx_ld_make(0.0L, p.r.low));
    size_t                  i = 0u;

    // the imaginary part along u
    for (i = 0u; i < 3u; ++i)
    {
        _out[i] = d_internal_math_ld_pair_of(
            d_internal_quat_ld_along(p.u[i], c.part[1]), 0.0L);
    }

    _out[3] = d_internal_math_ld_pair_of(c.part[0], 0.0L);

    return;
}

// d_internal_quat_ld_log_c
//   _out = log q = log|q| + u atan2(r, w), _q's vector part not along im alone
// and no part a NaN: cplx.h's logarithm at q's plane taken at 2^-k, plus
// r_lo i / z, and k ln 2 added to the real part as a pair (power.h's
// logarithm of 2^k). Where w > 0 and r < w 2^-(P/2 + 2), atan2(r, w) / r is
// 1 / w to the last place, and each part of v over w is the vector part.
D_MATH_FN_KERNEL void
d_internal_quat_ld_log_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 1);
    const struct dj_cplx_ld f = d_internal_cplx_ld_log_c(
        d_internal_cplx_ld_make(p.w, p.r.high));
    // d log z = dz / z
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, d_internal_cplx_ld_make(1.0L, 0.0L),
        d_internal_quat_ld_relative(p.w, p.r));
    const struct dj_internal_math_ld_pair k_ln2 = (p.scale == 0) ?
        d_internal_math_ld_pair_of(0.0L, 0.0L) :
        d_internal_math_ld_log_accurate_of(p.scale, 1.0L);
    const long double tiny =
        d_internal_math_ld_pow2(-((LDBL_MANT_DIG / 2) + 2));
    size_t            i    = 0u;

    // r far below w > 0: v / w
    if ( (p.w > 0.0L) &&
         (!d_internal_cplx_ld_is_inf(p.w)) &&
         (p.r.high < (p.w * tiny)) )
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(_q[i] / _q[3], 0.0L);
        }
    }
    else
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(
                d_internal_quat_ld_along(p.u[i], c.part[1]), 0.0L);
        }
    }

    // k ln 2 added where q was scaled, and so finite
    _out[3] = (p.scale == 0) ?
        d_internal_math_ld_pair_of(c.part[0], 0.0L) :
        d_internal_math_ld_pair_sum(
            d_internal_math_ld_pair_of(c.part[0], 0.0L), k_ln2);

    return;
}

// d_internal_quat_ld_sqrt_c
//   _out = sqrt q, the principal root, its real part >= 0: cplx.h's root at
// q's plane taken at 2^-2j, plus sqrt(z) (r_lo i / z) / 2, scaled back by
// 2^j; the vector part, where the real part is the larger, v / (2 Re), as
// 2 Re Im = r exactly, and otherwise Im along u. _q's vector part not along
// im alone and no part a NaN.
D_MATH_FN_KERNEL void
d_internal_quat_ld_sqrt_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 2);
    const struct dj_cplx_ld f = d_internal_cplx_ld_sqrt_c(
        d_internal_cplx_ld_make(p.w, p.r.high));
    const struct dj_cplx_ld rho = d_internal_quat_ld_relative(p.w, p.r);
    // d sqrt(z) = sqrt(z) (dz / z) / 2
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, d_internal_cplx_ld_make(f.part[0] / 2.0L, f.part[1] / 2.0L), rho);
    const int         half  = p.scale / 2;
    const long double re    = d_internal_cplx_ld_scale(c.part[0], half);
    const long double im    = d_internal_cplx_ld_scale(c.part[1], half);
    size_t            i     = 0u;

    // the real part the larger, in a finite q: v / (2 Re)
    if ( (d_internal_quat_ld_finite(_q, 4u)) &&
         (c.part[0] >= c.part[1]) &&
         (re > 0.0L) )
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(_q[i] / (2.0L * re), 0.0L);
        }
    }
    else
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = d_internal_math_ld_pair_of(
                d_internal_quat_ld_along(p.u[i], im), 0.0L);
        }
    }

    _out[3] = d_internal_math_ld_pair_of(re, 0.0L);

    return;
}

// d_internal_quat_ld_pow_c
//   _out = q^_p = exp(p log q), p real: cplx.h's power of q's plane point by
// p + 0i, plus p z^p (r_lo i / z), its imaginary part along u. _q's vector
// part not along im alone and no part a NaN; past |v| = LDBL_MAX, r is
// infinite and the value an infinite r's.
D_MATH_FN_KERNEL void
d_internal_quat_ld_pow_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _q,
    long double                      _p
)
{
    const struct dj_internal_quat_ld_plane p =
        d_internal_quat_ld_plane_c(_q, 0);
    const struct dj_cplx_ld f = d_internal_cplx_ld_pow_c(
        d_internal_cplx_ld_make(p.w, p.r.high),
        d_internal_cplx_ld_make(_p, 0.0L));
    // d z^p = p z^p dz / z
    const struct dj_cplx_ld c = d_internal_quat_ld_corrected(
        f, d_internal_cplx_ld_make(_p * f.part[0], _p * f.part[1]),
        d_internal_quat_ld_relative(p.w, p.r));
    size_t                  i = 0u;

    // the imaginary part along u
    for (i = 0u; i < 3u; ++i)
    {
        _out[i] = d_internal_math_ld_pair_of(
            d_internal_quat_ld_along(p.u[i], c.part[1]), 0.0L);
    }

    _out[3] = d_internal_math_ld_pair_of(c.part[0], 0.0L);

    return;
}

// d_internal_quat_ld_slerp_c
//   _out = the spherical linear interpolation from _a (t = 0) to _b (t = 1),
// both unit quaternions, along the shorter arc - b negated where a . b < 0:
// sin((1 - t) theta) / sin theta a + sin(t theta) / sin theta b, theta = 2
// atan2(|a - b|, |a + b|), the angle between them accurate near 0 as acos(a
// . b) is not; at theta = 0, (1 - t) a + t b (d_internal_quat_ld_lerp). Each
// part a pair of the two products. a at t = 0 and b at t = 1 exactly. A NaN
// or an infinite part, of t too: a NaN in each part.
D_MATH_FN_KERNEL void
d_internal_quat_ld_slerp_c(
    struct dj_internal_math_ld_pair* _out,
    const long double*               _a,
    const long double*               _b,
    long double                      _t
)
{
    long double b[4]    = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double diff[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double sum[4]  = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i       = 0u;

    // a NaN or an infinity: nothing to interpolate
    if ( (!d_internal_quat_ld_finite(_a, 4u)) ||
         (!d_internal_quat_ld_finite(_b, 4u)) ||
         (!d_internal_quat_ld_finite(&_t, 1u)) )
    {
        d_internal_quat_ld_fill_nan(_out, 4u);

        return;
    }

    {
        const long double flip = (d_internal_quat_ld_dot_sign(_a, _b, 4u) <
                                  0.0L) ? -1.0L : 1.0L;

        // the shorter arc's end, and the chords to it and through it
        for (i = 0u; i < 4u; ++i)
        {
            b[i]    = flip * _b[i];
            diff[i] = _a[i] - b[i];
            sum[i]  = _a[i] + b[i];
        }
    }

    {
        const long double theta = 2.0L * d_math_ld_atan2_c(
            d_internal_quat_ld_norm_c(diff, 4u).high,
            d_internal_quat_ld_norm_c(sum, 4u).high);

        // no angle: the line
        if (theta == 0.0L)
        {
            for (i = 0u; i < 4u; ++i)
            {
                _out[i] = d_internal_quat_ld_lerp(_a[i], b[i], _t);
            }

            return;
        }

        {
            const long double s    = d_math_ld_sin_c(theta);
            const long double rest = 1.0L - _t;
            const long double s_a  = d_math_ld_sin_c(rest * theta) / s;
            const long double s_b  = d_math_ld_sin_c(_t * theta) / s;
            const long double f[2] = { s_a, s_b };

            // each part of the arc
            for (i = 0u; i < 4u; ++i)
            {
                const long double ends[2] = { _a[i], b[i] };

                _out[i] = d_internal_quat_ld_dot(f, ends, 2u);
            }
        }
    }

    return;
}
"""


QUAT_FAMILY = r"""

//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// A quaternion is a caller's array [x, y, z, w]: x i + y j + z k + w. Each
// accurate function widens its operands to long double, exactly, takes
// section 1's kernel and rounds each part once; the _fast forms are the
// former formulas in the family's own type, for a renderer's inner loops; the
// plain forms of what takes a sine or a cosine are the same algorithms in the
// family's type on the C library's, and of what takes a square root alone
// the kernel on the library's root, which gives the same bits. _out may be
// an operand everywhere.


// @SECTION@.1    Helpers
//------------------------------------------------------------------------------

// d_internal_quat_@S@_widen
//   _out = _v's _n parts as long doubles, exactly.
D_MATH_FN void
d_internal_quat_@S@_widen(
    long double* _out,
    const @T@*   _v,
    size_t       _n
)
{
    size_t i = 0u;

    // each part, exactly
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = (long double)_v[i];
    }

    return;
}

// d_internal_quat_@S@_narrow
//   _out = each of _p's _n pairs rounded once to the family
// (d_internal_math_@S@_round_pair).
D_MATH_FN void
d_internal_quat_@S@_narrow(
    @T@*                                   _out,
    const struct dj_internal_math_ld_pair* _p,
    size_t                                 _n
)
{
    size_t i = 0u;

    // each pair, rounded once
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_math_@S@_round_pair(_p[i].high, _p[i].low);
    }

    return;
}

// d_internal_quat_@S@_all_finite
//   whether each of _v's _n parts is finite.
D_MATH_FN bool
d_internal_quat_@S@_all_finite(
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // a NaN or an infinity among the parts
    for (i = 0u; i < _n; ++i)
    {
        if ( (_v[i] != _v[i]) ||
             (d_internal_cplx_@S@_is_inf(_v[i])) )
        {
            return false;
        }
    }

    return true;
}

// d_internal_quat_@S@_all_zero
//   whether each of _v's _n parts is 0, of either sign.
D_MATH_FN bool
d_internal_quat_@S@_all_zero(
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // a part that is not 0
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != (@T@)0)
        {
            return false;
        }
    }

    return true;
}

#if @D2@
// d_internal_quat_@S@_widen_d2
//   _out = _v's _n parts as doubles, exactly.
D_MATH_FN void
d_internal_quat_@S@_widen_d2(
    double*    _out,
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // each part, exactly
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = (double)_v[i];
    }

    return;
}

@NARROW_D2@
#endif  // @D2@

// d_internal_quat_@S@_nans
//   _out = _n NaNs.
D_MATH_FN void
d_internal_quat_@S@_nans(
    @T@*   _out,
    size_t _n
)
{
    size_t i = 0u;

    // every part a NaN
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = d_internal_math_@S@_nan();
    }

    return;
}

// d_internal_quat_@S@_any_nan
//   whether any of _v's _n parts is a NaN.
D_MATH_FN bool
d_internal_quat_@S@_any_nan(
    const @T@* _v,
    size_t     _n
)
{
    size_t i = 0u;

    // a NaN among the parts
    for (i = 0u; i < _n; ++i)
    {
        if (_v[i] != _v[i])
        {
            return true;
        }
    }

    return false;
}

// d_internal_quat_@S@_part_along
//   _u _s, a zero where _u is 0, its sign _u's times _s's
// (d_internal_quat_ld_along).
D_MATH_FN @T@
d_internal_quat_@S@_part_along(
    @T@ _u,
    @T@ _s
)
{
    const @T@ zero = (@T@)0;
    const @T@ one  = (@T@)1;

    // a zero part: a zero, of the product's sign
    if (_u == zero)
    {
        const @T@ signed_zero = d_internal_cplx_@S@_copysign(zero, _s);

        return (d_internal_cplx_@S@_copysign(one, _u) < zero) ?
                   -signed_zero : signed_zero;
    }

    return _u * _s;
}

// dj_internal_quat_@S@_plain_plane
//   a quaternion's plane in the family (section 1.7), for the plain forms.
struct dj_internal_quat_@S@_plain_plane
{
    @T@ w;
    @T@ r;
    @T@ u[3];
};

// d_internal_quat_@S@_plain_plane_of
//   _q's plane, its vector part not 0 and no part a NaN: r = |v| by vec.h's
// scaled norm, u = v / r; a part of v infinite, r +inf and u the direction of
// v's infinities (d_internal_quat_ld_direction).
D_MATH_FN_RT struct dj_internal_quat_@S@_plain_plane
d_internal_quat_@S@_plain_plane_of(
    const @T@* _q
)
{
    struct dj_internal_quat_@S@_plain_plane result = {
        (@T@)0, (@T@)0,
        { (@T@)0, (@T@)0, (@T@)0 } };
    long double v[3] = { 0.0L, 0.0L, 0.0L };
    long double u[3] = { 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_@S@_widen(v, _q, 3u);
    result.w = _q[3];

    // an infinite part of v: an infinite r along v's infinities
    if (d_internal_quat_ld_has_inf(v, 3u))
    {
        d_internal_quat_ld_direction(u, v, 3u);
        result.r = d_internal_math_@S@_infinity();

        for (i = 0u; i < 3u; ++i)
        {
            result.u[i] = (@T@)u[i];
        }

        return result;
    }

    result.r = d_vec_@S@_norm(_q, 3u);

    // u = v / |v|
    for (i = 0u; i < 3u; ++i)
    {
        result.u[i] = _q[i] / result.r;
    }

    return result;
}


// @SECTION@.2    Algebra
//------------------------------------------------------------------------------
// Each part of the product, the inverse and the quotients is a pair's sum
// rounded once, as accurate as twice the pairs' precision: double's pairs
// where double is evaluated in its own type (section 1.3), a float's
// products exact in them; long double's for long double, and for float and
// double where double is evaluated wider, as on a 32-bit x87 (section 1.2).

// d_quat_@S@_multiply_fast
//   _out = _a _b by the formula, x = w x' + x w' + y z' - z y' and so on, 16
// products and 12 sums in the family's type, in order: for a renderer's inner
// loops, each part's error unbounded where its products cancel.
D_MATH_FN void
d_quat_@S@_multiply_fast(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    const @T@ ax = _a[0], ay = _a[1], az = _a[2], aw = _a[3];
    const @T@ bx = _b[0], by = _b[1], bz = _b[2], bw = _b[3];
    // x: wx' + xw' + yz' - zy'
    const @T@ x1 = aw * bx;
    const @T@ x2 = ax * bw;
    const @T@ x3 = ay * bz;
    const @T@ x4 = az * by;
    const @T@ x12 = x1 + x2;
    const @T@ x123 = x12 + x3;
    // y: wy' - xz' + yw' + zx'
    const @T@ y1 = aw * by;
    const @T@ y2 = ax * bz;
    const @T@ y3 = ay * bw;
    const @T@ y4 = az * bx;
    const @T@ y12 = y1 - y2;
    const @T@ y123 = y12 + y3;
    // z: wz' + xy' - yx' + zw'
    const @T@ z1 = aw * bz;
    const @T@ z2 = ax * by;
    const @T@ z3 = ay * bx;
    const @T@ z4 = az * bw;
    const @T@ z12 = z1 + z2;
    const @T@ z123 = z12 - z3;
    // w: ww' - xx' - yy' - zz'
    const @T@ w1 = aw * bw;
    const @T@ w2 = ax * bx;
    const @T@ w3 = ay * by;
    const @T@ w4 = az * bz;
    const @T@ w12 = w1 - w2;
    const @T@ w123 = w12 - w3;

    _out[0] = x123 - x4;
    _out[1] = y123 + y4;
    _out[2] = z123 + z4;
    _out[3] = w123 - w4;

    return;
}

// d_quat_@S@_multiply
//   _out = _a _b, Hamilton's product: the rotation _b, then _a. Each part
// the sum of its four products as Dot2 sums them, rounded once: in double's
// pairs where double is evaluated in its own type (section 1.3), long
// double's otherwise (section 1.2); where an operand has an infinite or NaN
// part, the formula as written (d_quat_@S@_multiply_fast).
D_MATH_FN void
d_quat_@S@_multiply(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // an infinity or a NaN: the formula as written
    if ( (!d_internal_quat_@S@_all_finite(_a, 4u)) ||
         (!d_internal_quat_@S@_all_finite(_b, 4u)) )
    {
        d_quat_@S@_multiply_fast(_out, _a, _b);

        return;
    }

#if @D2@
    {
        double                         a[4] = { 0.0, 0.0, 0.0, 0.0 };
        double                         b[4] = { 0.0, 0.0, 0.0, 0.0 };
        struct dj_internal_math_d_pair r[4] = { { 0.0, 0.0 }, { 0.0, 0.0 },
                                                { 0.0, 0.0 }, { 0.0, 0.0 } };

        d_internal_quat_@S@_widen_d2(a, _a, 4u);
        d_internal_quat_@S@_widen_d2(b, _b, 4u);
        d_internal_quat_d2_multiply(r, a, b);
        d_internal_quat_@S@_narrow_d2(_out, r, 4u);
    }
#else  // @D2@
    {
        long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
        struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L },
                                                 { 0.0L, 0.0L },
                                                 { 0.0L, 0.0L },
                                                 { 0.0L, 0.0L } };

        d_internal_quat_@S@_widen(a, _a, 4u);
        d_internal_quat_@S@_widen(b, _b, 4u);
        d_internal_quat_ld_multiply(r, a, b);
        d_internal_quat_@S@_narrow(_out, r, 4u);
    }
#endif  // @D2@

    return;
}

// d_quat_@S@_conjugate
//   _out = (-x, -y, -z, w): the inverse of a unit quaternion.
D_MATH_FN void
d_quat_@S@_conjugate(
    @T@*       _out,
    const @T@* _q
)
{
    _out[0] = -_q[0];
    _out[1] = -_q[1];
    _out[2] = -_q[2];
    _out[3] = _q[3];

    return;
}

// d_internal_quat_@S@_quotient
//   _out = _p / _q where _right, q^-1 p otherwise, and q's inverse where _p
// is NULL, both finite and q not 0: each part the kernel's pair rounded
// once, in double's pairs or long double's as d_quat_@S@_multiply takes
// them.
D_MATH_FN void
d_internal_quat_@S@_quotient(
    @T@*       _out,
    const @T@* _p,
    const @T@* _q,
    bool       _right
)
{
#if @D2@
    double                         p[4] = { 0.0, 0.0, 0.0, 0.0 };
    double                         q[4] = { 0.0, 0.0, 0.0, 0.0 };
    struct dj_internal_math_d_pair r[4] = { { 0.0, 0.0 }, { 0.0, 0.0 },
                                            { 0.0, 0.0 }, { 0.0, 0.0 } };

    d_internal_quat_@S@_widen_d2(q, _q, 4u);

    // the inverse: no numerator
    if (_p == NULL)
    {
        d_internal_quat_d2_divide(r, NULL, q, true);
    }
    else
    {
        d_internal_quat_@S@_widen_d2(p, _p, 4u);
        d_internal_quat_d2_divide(r, p, q, _right);
    }

    d_internal_quat_@S@_narrow_d2(_out, r, 4u);
#else  // @D2@
    long double                     p[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L },
                                             { 0.0L, 0.0L },
                                             { 0.0L, 0.0L },
                                             { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);

    // the inverse: no numerator
    if (_p == NULL)
    {
        d_internal_quat_ld_inverse(r, q);
    }
    else
    {
        d_internal_quat_@S@_widen(p, _p, 4u);
        d_internal_quat_ld_divide(r, p, q, _right);
    }

    d_internal_quat_@S@_narrow(_out, r, 4u);
#endif  // @D2@

    return;
}

// d_quat_@S@_inverse
//   _out = 1 / _q = q* / |q|^2, each part the kernel's pair rounded once
// (d_internal_quat_@S@_quotient). q = 0: a NaN in each part, the formula's 0
// / 0; a NaN part: NaNs; an infinite part, none a NaN: the zeros of q*'s
// parts' signs, as 1 / inf is 0.
D_MATH_FN void
d_quat_@S@_inverse(
    @T@*       _out,
    const @T@* _q
)
{
    long double q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    size_t      i    = 0u;

    d_internal_quat_@S@_widen(q, _q, 4u);

    // a NaN, or 0: no inverse
    if ( (d_internal_quat_ld_has_nan(q, 4u)) ||
         (d_internal_quat_ld_is_zero(q, 4u)) )
    {
        d_internal_quat_@S@_nans(_out, 4u);

        return;
    }

    // an infinity: the conjugate's zeros
    if (d_internal_quat_ld_has_inf(q, 4u))
    {
        for (i = 0u; i < 4u; ++i)
        {
            const long double part = (i < 3u) ? -q[i] : q[i];

            _out[i] = (@T@)d_internal_cplx_ld_copysign(0.0L, part);
        }

        return;
    }

    d_internal_quat_@S@_quotient(_out, NULL, _q, true);

    return;
}

// d_quat_@S@_divide
//   _out = _a / _b = a b^-1, the quotient on the right (S277): a b* / |b|^2,
// the numerator's parts as pairs over |b|^2's, each rounded once
// (d_internal_quat_@S@_quotient). Where an operand has an infinite or NaN
// part, or b is 0, a times b's inverse (d_quat_@S@_inverse), the product as
// written.
D_MATH_FN void
d_quat_@S@_divide(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // finite, b not 0: the kernel
    if ( (d_internal_quat_@S@_all_finite(_a, 4u)) &&
         (d_internal_quat_@S@_all_finite(_b, 4u)) &&
         (!d_internal_quat_@S@_all_zero(_b, 4u)) )
    {
        d_internal_quat_@S@_quotient(_out, _a, _b, true);

        return;
    }

    {
        const @T@ left[4]  = { _a[0], _a[1], _a[2], _a[3] };
        @T@       right[4] = { (@T@)0, (@T@)0,
                               (@T@)0, (@T@)0 };

        d_quat_@S@_inverse(right, _b);
        d_quat_@S@_multiply_fast(_out, left, right);
    }

    return;
}

// d_quat_@S@_left_divide
//   _out = _a \ _b = a^-1 b, the quotient on the left: a* b / |a|^2, as
// d_quat_@S@_divide computes its own; where an operand has an infinite or
// NaN part, or a is 0, a's inverse times b, the product as written.
D_MATH_FN void
d_quat_@S@_left_divide(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // finite, a not 0: the kernel
    if ( (d_internal_quat_@S@_all_finite(_a, 4u)) &&
         (d_internal_quat_@S@_all_finite(_b, 4u)) &&
         (!d_internal_quat_@S@_all_zero(_a, 4u)) )
    {
        d_internal_quat_@S@_quotient(_out, _b, _a, false);

        return;
    }

    {
        const @T@ right[4] = { _b[0], _b[1], _b[2], _b[3] };
        @T@       left[4]  = { (@T@)0, (@T@)0,
                               (@T@)0, (@T@)0 };

        d_quat_@S@_inverse(left, _a);
        d_quat_@S@_multiply_fast(_out, left, right);
    }

    return;
}


// @SECTION@.3    Norms
//------------------------------------------------------------------------------

// d_quat_@S@_norm_c, d_quat_@S@_norm
//   function: |q|, the kernel's pair rounded once (d_internal_quat_ld_norm_c):
// scaled, so that it overflows only where |q| does; +inf where a part is
// infinite, NaN where one is a NaN. The _c form a constant expression from
// C++14, the plain one on the C library's square root, the same value.
D_MATH_FN @T@
d_quat_@S@_norm_c(
    const @T@* _q
)
{
    long double                          q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair      n    = { 0.0L, 0.0L };

    d_internal_quat_@S@_widen(q, _q, 4u);
    n = d_internal_quat_ld_norm_c(q, 4u);

    return d_internal_math_@S@_round_pair(n.high, n.low);
}

D_MATH_FN_RT @T@
d_quat_@S@_norm(
    const @T@* _q
)
{
    long double                          q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair      n    = { 0.0L, 0.0L };

    d_internal_quat_@S@_widen(q, _q, 4u);
    n = d_internal_quat_ld_norm(q, 4u);

    return d_internal_math_@S@_round_pair(n.high, n.low);
}

// d_quat_@S@_normalize_c, d_quat_@S@_normalize
//   _out = _q / |q|, each part the kernel's pair rounded once
// (d_internal_quat_ld_normalize_c), q scaled so that nothing overflows. A
// zero q is left as it is, signs of zero included; a NaN part gives NaNs, an
// infinite one the direction of q's infinities. The _c form a constant
// expression from C++14, the plain one on the C library's square root, the
// same value.
D_MATH_FN void
d_quat_@S@_normalize_c(
    @T@*       _out,
    const @T@* _q
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);
    d_internal_quat_ld_normalize_c(r, q);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_normalize(
    @T@*       _out,
    const @T@* _q
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);
    d_internal_quat_ld_normalize(r, q);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_normalize_fast
//   _out = _q times 1 / sqrt(x^2 + y^2 + z^2 + w^2), the reciprocal square
// root a renderer's normalization takes, in the family's type and unscaled:
// for |q|^2 within the type's range. A zero q is left as it is.
D_MATH_FN_RT void
d_quat_@S@_normalize_fast(
    @T@*       _out,
    const @T@* _q
)
{
    const @T@ xx  = _q[0] * _q[0];
    const @T@ yy  = _q[1] * _q[1];
    const @T@ zz  = _q[2] * _q[2];
    const @T@ ww  = _q[3] * _q[3];
    const @T@ xy  = xx + yy;
    const @T@ xyz = xy + zz;
    const @T@ sum = xyz + ww;
    size_t    i   = 0u;

    // a zero quaternion: left as it is
    if (sum == (@T@)0)
    {
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = _q[i];
        }

        return;
    }

    {
        const @T@ s = (@T@)1 / d_math_@S@_sqrt(sum);

        // each part by the reciprocal root
        for (i = 0u; i < 4u; ++i)
        {
            _out[i] = _q[i] * s;
        }
    }

    return;
}


// @SECTION@.4    Rotation
//------------------------------------------------------------------------------
// A rotation is q v q^-1, active and right-handed, as glTF's, Eigen's and
// GLM's (S276): for a unit q, q v q*. The accurate forms take any q not 0,
// as its norm cancels; the _fast forms assume a unit q, as before.

// d_quat_@S@_rotate_fast
//   _out = _v rotated by the unit quaternion _q, v + w t + u x t with u the
// vector part and t = 2 (u x v), in the family's type, without building a
// matrix.
D_MATH_FN void
d_quat_@S@_rotate_fast(
    @T@*       _out,
    const @T@* _q,
    const @T@* _v
)
{
    @T@ t[3]  = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ wt[3] = { (@T@)0, (@T@)0, (@T@)0 };
    @T@ ut[3] = { (@T@)0, (@T@)0, (@T@)0 };

    d_vec_@S@_cross_fast(t, _q, _v);
    d_vec_@S@_scale(t, t, (@T@)2, 3u);
    d_vec_@S@_scale(wt, t, _q[3], 3u);
    d_vec_@S@_cross_fast(ut, _q, t);
    d_vec_@S@_add(_out, _v, wt, 3u);
    d_vec_@S@_add(_out, _out, ut, 3u);

    return;
}

// d_quat_@S@_rotate
//   _out = q v q^-1, the 3-vector _v turned by the rotation _q names, for any
// q not 0: each part the kernel's pair rounded once
// (d_internal_quat_ld_rotate). v = 0 is left as it is; q = 0, or a NaN or
// infinite part, gives a NaN in each part.
D_MATH_FN void
d_quat_@S@_rotate(
    @T@*       _out,
    const @T@* _q,
    const @T@* _v
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     v[3] = { 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[3] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L } };
    size_t                          i    = 0u;

    d_internal_quat_@S@_widen(q, _q, 4u);
    d_internal_quat_@S@_widen(v, _v, 3u);

    // no rotation: a zero q, a NaN or an infinity
    if ( (!d_internal_quat_ld_finite(q, 4u)) ||
         (!d_internal_quat_ld_finite(v, 3u)) ||
         (d_internal_quat_ld_is_zero(q, 4u)) )
    {
        d_internal_quat_@S@_nans(_out, 3u);

        return;
    }

    // the zero vector: as it is
    if (d_internal_quat_ld_is_zero(v, 3u))
    {
        for (i = 0u; i < 3u; ++i)
        {
            _out[i] = _v[i];
        }

        return;
    }

    d_internal_quat_ld_rotate(r, q, v);
    d_internal_quat_@S@_narrow(_out, r, 3u);

    return;
}

// d_quat_@S@_to_matrix3_fast
//   _out (3 x 3, by rows) = the rotation matrix of the unit quaternion _q, 1
// - 2 (y^2 + z^2) and so on, in the family's type.
D_MATH_FN void
d_quat_@S@_to_matrix3_fast(
    @T@*       _out,
    const @T@* _q
)
{
    const @T@ x  = _q[0];
    const @T@ y  = _q[1];
    const @T@ z  = _q[2];
    const @T@ w  = _q[3];
    const @T@ xx = x * x;
    const @T@ yy = y * y;
    const @T@ zz = z * z;
    const @T@ xy = x * y;
    const @T@ xz = x * z;
    const @T@ yz = y * z;
    const @T@ wx = w * x;
    const @T@ wy = w * y;
    const @T@ wz = w * z;
    // the sums and differences the entries double
    const @T@ yy_zz = yy + zz;
    const @T@ xx_zz = xx + zz;
    const @T@ xx_yy = xx + yy;
    const @T@ xy_m  = xy - wz;
    const @T@ xy_p  = xy + wz;
    const @T@ xz_p  = xz + wy;
    const @T@ xz_m  = xz - wy;
    const @T@ yz_m  = yz - wx;
    const @T@ yz_p  = yz + wx;
    const @T@ d00   = (@T@)2 * yy_zz;
    const @T@ d11   = (@T@)2 * xx_zz;
    const @T@ d22   = (@T@)2 * xx_yy;

    _out[0] = (@T@)1 - d00;
    _out[1] = (@T@)2 * xy_m;
    _out[2] = (@T@)2 * xz_p;
    _out[3] = (@T@)2 * xy_p;
    _out[4] = (@T@)1 - d11;
    _out[5] = (@T@)2 * yz_m;
    _out[6] = (@T@)2 * xz_m;
    _out[7] = (@T@)2 * yz_p;
    _out[8] = (@T@)1 - d22;

    return;
}

// d_quat_@S@_to_matrix3
//   _out (3 x 3, by rows) = the rotation matrix of _q, any q not 0, each
// entry the kernel's pair rounded once (d_internal_quat_ld_to_matrix3). q =
// 0, or a NaN or infinite part, gives NaN entries.
D_MATH_FN void
d_quat_@S@_to_matrix3(
    @T@*       _out,
    const @T@* _q
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[9] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(q, _q, 4u);

    // no rotation: a zero q, a NaN or an infinity
    if ( (!d_internal_quat_ld_finite(q, 4u)) ||
         (d_internal_quat_ld_is_zero(q, 4u)) )
    {
        d_internal_quat_@S@_nans(_out, 9u);

        return;
    }

    d_internal_quat_ld_to_matrix3(r, q);
    d_internal_quat_@S@_narrow(_out, r, 9u);

    return;
}

// d_quat_@S@_to_matrix4_fast, d_quat_@S@_to_matrix4
//   _out (4 x 4, by rows) = the homogeneous rotation matrix: the 3 x 3 of
// d_quat_@S@_to_matrix3_fast or d_quat_@S@_to_matrix3 in its upper left, the
// identity's last row and column.
D_MATH_FN void
d_quat_@S@_to_matrix4_fast(
    @T@*       _out,
    const @T@* _q
)
{
    @T@    r[9] = { (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0 };
    size_t i    = 0u;

    d_quat_@S@_to_matrix3_fast(r, _q);
    d_mat_@S@_identity(_out, 4u);

    // the 3 x 3 into the upper left
    for (i = 0u; i < 3u; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < 3u; ++j)
        {
            _out[(i * 4u) + j] = r[(i * 3u) + j];
        }
    }

    return;
}

D_MATH_FN void
d_quat_@S@_to_matrix4(
    @T@*       _out,
    const @T@* _q
)
{
    @T@    r[9] = { (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0, (@T@)0 };
    size_t i    = 0u;

    d_quat_@S@_to_matrix3(r, _q);
    d_mat_@S@_identity(_out, 4u);

    // the 3 x 3 into the upper left
    for (i = 0u; i < 3u; ++i)
    {
        size_t j = 0u;

        for (j = 0u; j < 3u; ++j)
        {
            _out[(i * 4u) + j] = r[(i * 3u) + j];
        }
    }

    return;
}

// d_quat_@S@_from_matrix3_c, d_quat_@S@_from_matrix3
//   _out = the unit quaternion, w >= 0, of the rotation matrix _m (3 x 3, by
// rows), by Shepperd's method (d_internal_quat_ld_from_matrix3_c), each part
// rounded once: to_matrix3's inverse, q or -q. A NaN or infinite entry gives
// NaNs. The _c form a constant expression from C++14, the plain one on the C
// library's square root, the same value.
D_MATH_FN void
d_quat_@S@_from_matrix3_c(
    @T@*       _out,
    const @T@* _m
)
{
    long double                     m[9] = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L,
                                             0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(m, _m, 9u);
    d_internal_quat_ld_from_matrix3_c(r, m);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_from_matrix3(
    @T@*       _out,
    const @T@* _m
)
{
    long double                     m[9] = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L,
                                             0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(m, _m, 9u);
    d_internal_quat_ld_from_matrix3(r, m);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_from_matrix4_c, d_quat_@S@_from_matrix4
//   _out = the unit quaternion of the 4 x 4 matrix _m's upper-left 3 x 3
// (d_quat_@S@_from_matrix3_c).
D_MATH_FN void
d_quat_@S@_from_matrix4_c(
    @T@*       _out,
    const @T@* _m
)
{
    const @T@ m[9] = { _m[0], _m[1], _m[2],
                       _m[4], _m[5], _m[6],
                       _m[8], _m[9], _m[10] };

    d_quat_@S@_from_matrix3_c(_out, m);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_from_matrix4(
    @T@*       _out,
    const @T@* _m
)
{
    const @T@ m[9] = { _m[0], _m[1], _m[2],
                       _m[4], _m[5], _m[6],
                       _m[8], _m[9], _m[10] };

    d_quat_@S@_from_matrix3(_out, m);

    return;
}

// d_quat_@S@_from_two_vectors_c, d_quat_@S@_from_two_vectors
//   _out = the unit quaternion of the shortest rotation turning the direction
// of the 3-vector _a into _b's (d_internal_quat_ld_from_two_vectors_c), each
// part rounded once: exactly opposed, a half turn about an axis
// perpendicular to a; a zero vector, the identity; a NaN or infinite part,
// NaNs. The _c form a constant expression from C++14, the plain one on the C
// library's square root, the same value.
D_MATH_FN void
d_quat_@S@_from_two_vectors_c(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    long double                     a[3] = { 0.0L, 0.0L, 0.0L };
    long double                     b[3] = { 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 3u);
    d_internal_quat_@S@_widen(b, _b, 3u);
    d_internal_quat_ld_from_two_vectors_c(r, a, b);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_from_two_vectors(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    long double                     a[3] = { 0.0L, 0.0L, 0.0L };
    long double                     b[3] = { 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 3u);
    d_internal_quat_@S@_widen(b, _b, 3u);
    d_internal_quat_ld_from_two_vectors(r, a, b);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}


// @SECTION@.5    Construction and interpolation
//------------------------------------------------------------------------------
// What needs a sine or a cosine comes twice, as the elementary functions do:
// the _c form a constant expression from C++14, the plain one on the C
// library's.
@TRIG_FORMS@
// d_quat_@S@_nlerp_c, d_quat_@S@_nlerp
//   _out = the normalized linear interpolation from _a (t = 0) to _b (t = 1),
// along the shorter arc (d_internal_quat_ld_nlerp_c), each part rounded once:
// the cheap curve between two rotations, whose speed varies along it. A NaN
// or infinite part, of t too, gives NaNs. The _c form a constant expression
// from C++14, the plain one on the C library's square root, the same value.
D_MATH_FN void
d_quat_@S@_nlerp_c(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 4u);
    d_internal_quat_@S@_widen(b, _b, 4u);
    d_internal_quat_ld_nlerp_c(r, a, b, (long double)_t);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_nlerp(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 4u);
    d_internal_quat_@S@_widen(b, _b, 4u);
    d_internal_quat_ld_nlerp(r, a, b, (long double)_t);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_slerp_c
//   _out = the spherical linear interpolation from _a (t = 0) to _b (t = 1),
// both unit quaternions, along the shorter arc at a constant speed
// (d_internal_quat_ld_slerp_c), each part rounded once: the angle from
// atan2 of the chords, accurate near 0, and a at t = 0, b or -b at t = 1,
// exactly. A NaN or infinite part, of t too, gives NaNs.
D_MATH_FN void
d_quat_@S@_slerp_c(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    long double                     a[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    long double                     b[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    d_internal_quat_@S@_widen(a, _a, 4u);
    d_internal_quat_@S@_widen(b, _b, 4u);
    d_internal_quat_ld_slerp_c(r, a, b, (long double)_t);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

// d_quat_@S@_slerp
//   the same algorithm in the family's type on the C library's: theta = 2
// atan2(|a - b|, |a + b|) by vec.h's scaled norms, the sines the library's,
// each part the sum of the two products; NaNs where the arithmetic gives
// them.
D_MATH_FN_RT void
d_quat_@S@_slerp(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t
)
{
    const @T@ zero    = (@T@)0;
    const @T@ one     = (@T@)1;
    @T@       b[4]    = { zero, zero, zero, zero };
    @T@       diff[4] = { zero, zero, zero, zero };
    @T@       sum[4]  = { zero, zero, zero, zero };
    const @T@ d       = d_vec_@S@_dot(_a, _b, 4u);
    const @T@ flip    = (d < zero) ? -one : one;
    size_t    i       = 0u;

    // the shorter arc's end, and the chords to it and through it
    for (i = 0u; i < 4u; ++i)
    {
        b[i]    = flip * _b[i];
        diff[i] = _a[i] - b[i];
        sum[i]  = _a[i] + b[i];
    }

    {
        const @T@ chord = d_vec_@S@_norm(diff, 4u);
        const @T@ other = d_vec_@S@_norm(sum, 4u);
        const @T@ theta = (one + one) * d_math_@S@_atan2(chord, other);
        @T@       s_a   = one - _t;
        @T@       s_b   = _t;

        // an angle: the arc's sines
        if (theta != zero)
        {
            const @T@ s    = d_math_@S@_sin(theta);
            const @T@ rest = one - _t;

            s_a = d_math_@S@_sin(rest * theta) / s;
            s_b = d_math_@S@_sin(_t * theta) / s;
        }

        // each part of the arc
        for (i = 0u; i < 4u; ++i)
        {
            const @T@ from = s_a * _a[i];
            const @T@ to   = s_b * b[i];

            _out[i] = from + to;
        }
    }

    return;
}


// @SECTION@.6    Exponential, logarithm and powers
//------------------------------------------------------------------------------
// The complex functions in q's plane (section 1.7), principal values (S275):
// for a vector part along im alone - y and z 0 - the complex function of w
// + x i, cplx.h's in the same form, y and z as they are, so that a complex
// number's is its own bit for bit; otherwise the _c form the kernel rounded
// once, the plain one the same in the family's type on cplx.h's plain
// functions. A NaN part, there, gives NaNs.
@EXP_FORMS@"""


QUAT_TRIG = r"""
// d_quat_@S@_from_axis_angle@F@
//   _out = the rotation of _angle radians about _axis, which need not be a
// unit vector: (axis sin(a/2) / |axis|, cos(a/2)). A zero axis names no
// rotation: the identity.
D_MATH_FN@RT@ void
d_quat_@S@_from_axis_angle@F@(
    @T@*       _out,
    const @T@* _axis,
    @T@        _angle
)
{
    const @T@ length = d_vec_@S@_norm_c(_axis, 3u);

    // a zero axis names no rotation
    if (length == (@T@)0)
    {
        _out[0] = (@T@)0;
        _out[1] = (@T@)0;
        _out[2] = (@T@)0;
        _out[3] = (@T@)1;
    }
    else
    {
        const @T@ half = _angle / (@T@)2;
        const @T@ sine = d_math_@S@_sin@F@(half);
        const @T@ s    = sine / length;

        _out[0] = _axis[0] * s;
        _out[1] = _axis[1] * s;
        _out[2] = _axis[2] * s;
        _out[3] = d_math_@S@_cos@F@(half);
    }

    return;
}

// d_quat_@S@_from_unit_axis_angle@F@
//   _out = (axis sin(a/2), cos(a/2)) for an axis the caller normalized (a
// zero one gives (0, 0, 0, cos(a/2))).
D_MATH_FN@RT@ void
d_quat_@S@_from_unit_axis_angle@F@(
    @T@*       _out,
    const @T@* _axis,
    @T@        _angle
)
{
    const @T@ half = _angle / (@T@)2;
    const @T@ s    = d_math_@S@_sin@F@(half);

    _out[0] = _axis[0] * s;
    _out[1] = _axis[1] * s;
    _out[2] = _axis[2] * s;
    _out[3] = d_math_@S@_cos@F@(half);

    return;
}
"""


# one of the four functions of section @SECTION@.6, in both forms: @FN@ the
# name, @DOC@ its line, @ARGS@ and @PARAMS@ its extra argument, @KARGS@ the
# kernel's, @CPLX_EXTRA@ the complex function's.
QUAT_EXP = r"""
// d_quat_@S@_@FN@_c, d_quat_@S@_@FN@
//   _out = @DOC@
D_MATH_FN void
d_quat_@S@_@FN@_c(
    @T@*       _out,
    const @T@* _q@PARAMS@
)
{
    long double                     q[4] = { 0.0L, 0.0L, 0.0L, 0.0L };
    struct dj_internal_math_ld_pair r[4] = { { 0.0L, 0.0L }, { 0.0L, 0.0L },
                                             { 0.0L, 0.0L }, { 0.0L, 0.0L } };

    // along im alone: the complex function
    if ( (_q[1] == (@T@)0) &&
         (_q[2] == (@T@)0) )
    {
        const struct dj_cplx_@S@ c = d_cplx_@S@_@FN@_c(
            d_cplx_@S@_make(_q[3], _q[0])@CPLX_EXTRA@);

        _out[0] = c.part[1];
        _out[1] = _q[1];
        _out[2] = _q[2];
        _out[3] = c.part[0];

        return;
    }

    d_internal_quat_@S@_widen(q, _q, 4u);

    // a NaN: NaNs
    if ( (d_internal_quat_ld_has_nan(q, 4u))@NAN_EXTRA@ )
    {
        d_internal_quat_@S@_nans(_out, 4u);

        return;
    }

    d_internal_quat_ld_@FN@_c(r, q@KARGS@);
    d_internal_quat_@S@_narrow(_out, r, 4u);

    return;
}

D_MATH_FN_RT void
d_quat_@S@_@FN@(
    @T@*       _out,
    const @T@* _q@PARAMS@
)
{
    @T@    r[4] = { (@T@)0, (@T@)0,
                    (@T@)0, (@T@)0 };
    size_t i    = 0u;

    // along im alone: the complex function
    if ( (_q[1] == (@T@)0) &&
         (_q[2] == (@T@)0) )
    {
        const struct dj_cplx_@S@ c = d_cplx_@S@_@FN@(
            d_cplx_@S@_make(_q[3], _q[0])@CPLX_EXTRA@);

        _out[0] = c.part[1];
        _out[1] = _q[1];
        _out[2] = _q[2];
        _out[3] = c.part[0];

        return;
    }

    // a NaN: NaNs
    if ( (d_internal_quat_@S@_any_nan(_q, 4u))@NAN_EXTRA@ )
    {
        d_internal_quat_@S@_nans(_out, 4u);

        return;
    }

    {
        const struct dj_internal_quat_@S@_plain_plane p =
            d_internal_quat_@S@_plain_plane_of(_q);
        const struct dj_cplx_@S@ c = d_cplx_@S@_@FN@(
            d_cplx_@S@_make(p.w, p.r)@CPLX_EXTRA@);
@PLAIN_BODY@
        r[3] = c.part[0];
    }

    // the parts, after every read of _q
    for (i = 0u; i < 4u; ++i)
    {
        _out[i] = r[i];
    }

    return;
}
"""

QUAT_EXP_ALONG = r"""
        // the imaginary part along u
        for (i = 0u; i < 3u; ++i)
        {
            r[i] = d_internal_quat_@S@_part_along(p.u[i], c.part[1]);
        }
"""

QUAT_LOG_PLAIN = r"""
        const @T@ tiny =
            d_internal_math_@S@_pow2(-((@P@ / 2) + 2));

        // r far below w > 0: v / w, atan2(r, w) / r being 1 / w
        if ( (p.w > (@T@)0) &&
             (!d_internal_cplx_@S@_is_inf(p.w)) &&
             (p.r < (p.w * tiny)) )
        {
            for (i = 0u; i < 3u; ++i)
            {
                r[i] = _q[i] / _q[3];
            }
        }
        else
        {
            for (i = 0u; i < 3u; ++i)
            {
                r[i] = d_internal_quat_@S@_part_along(p.u[i], c.part[1]);
            }
        }
"""

QUAT_SQRT_PLAIN = r"""
        // the real part the larger, q finite: v / (2 Re), as 2 Re Im = r
        if ( (!d_internal_cplx_@S@_is_inf(p.r)) &&
             (!d_internal_cplx_@S@_is_inf(p.w)) &&
             (c.part[0] >= c.part[1]) &&
             (c.part[0] > (@T@)0) )
        {
            const @T@ twice = (@T@)2 * c.part[0];

            for (i = 0u; i < 3u; ++i)
            {
                r[i] = _q[i] / twice;
            }
        }
        else
        {
            for (i = 0u; i < 3u; ++i)
            {
                r[i] = d_internal_quat_@S@_part_along(p.u[i], c.part[1]);
            }
        }
"""

QUAT_EXP_SPECS = [
    # name, doc, params, kernel args, complex extra, nan extra, plain body
    ("exp", "e^q = e^w (cos |v| + u sin |v|), u = v / |v|.",
     "", "", "", "", QUAT_EXP_ALONG),
    ("log", "log q = log |q| + u atan2(|v|, w), the principal logarithm,\n"
            "// u = im where v is 0, so that log(-1) is pi im (S275).",
     "", "", "", "", QUAT_LOG_PLAIN),
    ("sqrt", "sqrt q, the principal square root, its real part >= 0: the\n"
             "// complex root of w + |v| i, its imaginary part along u.",
     "", "", "", "", QUAT_SQRT_PLAIN),
    ("pow", "q^_p = exp(p log q) for a real p, the principal power: the\n"
            "// complex one of w + |v| i, its imaginary part along u.",
     ",\n    @T@        _p", ", (long double)_p",
     ",\n            d_cplx_@S@_make(_p, (@T@)0)", " ||\n         (_p != _p)",
     QUAT_EXP_ALONG),
]


QUAT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS
    -----------
    1.  Pairs and scaling
    2.  The algebra
    3.  The algebra in double
    4.  Rotation
    5.  Roots: the _c forms'
    6.  Roots: the plain forms'
    7.  The _c forms' functions
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Helpers
    2.  Algebra
    3.  Norms
    4.  Rotation
    5.  Construction and interpolation
    6.  Exponential, logarithm and powers
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Helpers
    2.  Algebra
    3.  Norms
    4.  Rotation
    5.  Construction and interpolation
    6.  Exponential, logarithm and powers
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Helpers
    2.  Algebra
    3.  Norms
    4.  Rotation
    5.  Construction and interpolation
    6.  Exponential, logarithm and powers
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_QUAT_H
#define DJINTERP_C_MATH_QUAT_H 1

// std
#include <float.h>           // LDBL_MANT_DIG, LDBL_MIN, LDBL_MIN_EXP, ...
#include <stddef.h>          // size_t
// djinterp
#include "./cplx.h"          // d_cplx_*_exp, _log, _sqrt, _pow, their kernels
#include "./elementary.h"    // d_math_*_sin, _cos, _sqrt, pairs, round_pair
#include "./exponential.h"   // d_internal_math_*_infinity
#include "./float_order.h"   // d_internal_math_*_pow2
#include "./mat.h"           // d_mat_*_identity
#include "./math_common.h"   // D_MATH_FN, D_MATH_FN_KERNEL, D_MATH_FN_RT
#include "./power.h"         // d_internal_math_ld_pair_sum, _pair_product
#include "./tangent.h"       // d_math_*_atan2 and its _c form
#include "./vec.h"           // d_vec_*_cross, _dot, _norm, _scale, _add
"""

QUAT_TAIL = r"""

#endif  // DJINTERP_C_MATH_QUAT_H
"""

QUAT_ROOT_FORMS = (
    (5, "_c", "D_MATH_FN_KERNEL", "d_math_ld_sqrt_c", "Roots: the _c forms'",
     "What takes a square root and no other real function, on the core's\n"
     "// _c root: a constant expression from C++14."),
    (6, "", "D_MATH_FN_RT", "d_math_ld_sqrt", "Roots: the plain forms'",
     "The same, on the C library's square root, correctly rounded as the\n"
     "// _c root is: the same bits, faster."),
)

_QO = ("@T@*", "_out")
_QQ = ("const @T@*", "_q")
_QA = ("const @T@*", "_a")
_QB = ("const @T@*", "_b")
_QV = ("const @T@*", "_v")
_QM = ("const @T@*", "_m")
QUAT_OVERLOADS = [
    ("D_MATH_FN", "void", "d_quat_multiply_fast", [_QO, _QA, _QB]),
    ("D_MATH_FN", "void", "d_quat_multiply", [_QO, _QA, _QB]),
    ("D_MATH_FN", "void", "d_quat_conjugate", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_inverse", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_divide", [_QO, _QA, _QB]),
    ("D_MATH_FN", "void", "d_quat_left_divide", [_QO, _QA, _QB]),
    ("D_MATH_FN", "@T@", "d_quat_norm_c", [_QQ]),
    ("D_MATH_FN_RT", "@T@", "d_quat_norm", [_QQ]),
    ("D_MATH_FN", "void", "d_quat_normalize_c", [_QO, _QQ]),
    ("D_MATH_FN_RT", "void", "d_quat_normalize", [_QO, _QQ]),
    ("D_MATH_FN_RT", "void", "d_quat_normalize_fast", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_rotate_fast", [_QO, _QQ, _QV]),
    ("D_MATH_FN", "void", "d_quat_rotate", [_QO, _QQ, _QV]),
    ("D_MATH_FN", "void", "d_quat_to_matrix3_fast", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_to_matrix3", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_to_matrix4_fast", [_QO, _QQ]),
    ("D_MATH_FN", "void", "d_quat_to_matrix4", [_QO, _QQ]),
]
for _f, _m in (("_c", "D_MATH_FN"), ("", "D_MATH_FN_RT")):
    QUAT_OVERLOADS += [
        (_m, "void", "d_quat_from_matrix3" + _f, [_QO, _QM]),
        (_m, "void", "d_quat_from_matrix4" + _f, [_QO, _QM]),
        (_m, "void", "d_quat_from_two_vectors" + _f, [_QO, _QA, _QB]),
        (_m, "void", "d_quat_from_axis_angle" + _f, [_QO, ("const @T@*", "_axis"), ("@T@", "_angle")]),
        (_m, "void", "d_quat_from_unit_axis_angle" + _f, [_QO, ("const @T@*", "_axis"), ("@T@", "_angle")]),
        (_m, "void", "d_quat_nlerp" + _f, [_QO, _QA, _QB, ("@T@", "_t")]),
        (_m, "void", "d_quat_slerp" + _f, [_QO, _QA, _QB, ("@T@", "_t")]),
        (_m, "void", "d_quat_exp" + _f, [_QO, _QQ]),
        (_m, "void", "d_quat_log" + _f, [_QO, _QQ]),
        (_m, "void", "d_quat_sqrt" + _f, [_QO, _QQ]),
        (_m, "void", "d_quat_pow" + _f, [_QO, _QQ, ("@T@", "_p")]),
    ]


def quat():
    # (a blank line before it, its text): each wrapped to 72 columns
    paragraphs = [
        (False,
         'Quaternions: the C core of math/quaternion.hpp and '
         "math/linear_algebra/quaternion.hpp, and of the language's "
         'quaternions, for float, double and long double.'),
        (False,
         'GENERATED by scripts/gen_c_math_float.py from one text for the '
         'three families: edit the generator, not this file.'),
        (True,
         "A quaternion here is a caller's array [x, y, z, w], x i + y j + "
         'z k + w, the order of glTF, Eigen and GLM, so that a renderer '
         "takes one as it is (S275); the product is Hamilton's, "
         "right-handed, and a rotation active, v' = q v q^-1, q v q* for a "
         'unit q (S276).'),
        (True,
         'Accurate by default (S276): the product, the inverse and the two '
         'quotients, the rotation and its matrix, the norm and '
         "normalization, the quaternion of a rotation matrix (Shepperd's "
         'method) and of two vectors, and nlerp are each one long double '
         'kernel, its parts carried as pairs where they cancel - every '
         "product of two parts exact by Dekker's, every sum of products "
         "Ogita, Rump and Oishi's Dot2 - and each family's part the "
         "kernel's rounded once, the operands scaled by powers of 2 where "
         "Dekker's products could overflow or lose their low parts; "
         "float's and double's product, inverse and quotients the same in "
         "double's pairs where double is evaluated in its own type, as SSE "
         "evaluates it, some ten times faster than the x87's long double. "
         "The product's parts are as accurate as their sums computed in "
         "twice the pairs' precision and rounded once. The rotation and "
         'its matrix take any q not 0, its norm cancelling, so that a q a '
         'few steps from unit length names the rotation of q / |q|. What '
         'takes a root and no other real function comes twice, the _c form '
         'a constant expression from C++14 and the plain one on the C '
         "library's root, the two giving the same bits."),
        (True,
         'slerp, exp, log, sqrt and real powers come twice, as the '
         'elementary functions do: the _c form one long double kernel on '
         "the core's _c functions; the plain form the same algorithm in "
         "the family's type on the C library's. slerp along the shorter "
         'arc, its angle 2 atan2(|a - b|, |a + b|), accurate near 0 where '
         'acos(a . b) is not, a at t = 0 and b at t = 1 exactly. exp, log, '
         "sqrt and pow are the complex functions in q's plane, principal "
         'values (S275): f(q) = Re f(w + r i) + u Im f(w + r i), r = |v|, '
         "u = v / r; for v along im alone, cplx.h's function itself, bit "
         'for bit, y and z as they are; log(-1) is pi im. In the _c '
         'kernels r is a pair and f corrected to first order for its low '
         'part, and log and sqrt take q scaled by a power of 2 where it '
         "leaves the range Dekker's products need."),
        (True,
         "The former formulas stay, in the family's type, as the _fast "
         'forms of the product, the rotation, its matrices and '
         "normalization (by a reciprocal square root), for a renderer's "
         "inner loops: their bits are the earlier header's, each part's "
         'error unbounded where products cancel. from_axis_angle and '
         'from_unit_axis_angle are as before.'),
        (True,
         'Where a part is infinite or a NaN: the product as written (the '
         "_fast formula); the quotients a times b's inverse, which is the "
         "conjugate's zeros for an infinite q and NaNs for a zero or NaN "
         "one; NaNs from the rotation, its matrix, Shepperd's method, two "
         'vectors, nlerp and slerp; the direction of the infinities from '
         "normalization, +inf from the norm; and in q's plane, an infinite "
         "r along v's infinities, a NaN part giving NaNs, a part of v that "
         'is 0 staying 0.'),
        (True,
         'The bounds, in steps (float_order.h) of each part from the part '
         'correctly rounded, as tests/djinterp/c/modules/math/t_quat.c checks them '
         'at the points of math_tests_quat_refs.h, computed by mpmath: '
         'points exact in float, which every family takes, at the ends of '
         "each family's range, and at double's and long double's full "
         'width. The product within 1 but where its sum cancels, by u^2 '
         "times its terms' magnitudes (measured 0, but 8 for double where "
         'a sum cancels to 2^-50 of its terms); the inverse, the two '
         'quotients, the rotation and its matrix, the norm, normalization '
         "and Shepperd's method within 1 (measured 0, but 1 for long "
         "double's rotation); two vectors' quaternion and nlerp within 1 "
         'for float and double and 2 for long double (measured 0, 0 and '
         '1); slerp_c within 1 step of its largest part, as its parts are '
         'sums that may cancel, and 2 for long double (measured 0, 0 and '
         '2); exp_c, log_c, sqrt_c and pow_c within 1 for float and double '
         'and 4 for long double (measured 0, and 2 each), a whole power in '
         "q's plane being cplx.h's, carried in pairs (S370). The plain "
         'slerp within 4 steps of its largest part (measured 2); the plain '
         "exp, log, sqrt and pow, which take |v| rounded once in the "
         "family's type, within 12, 4, 4 and 12 steps of the largest part "
         'where |v| is below 16 and |p| below 8 (measured 8, 3, 2 and 5): '
         "exp's and pow's grow with |v| and p, as the cosine of a rounded "
         '|v| does.'),
    ]
    lines = []
    for n, (blank, paragraph) in enumerate(paragraphs):
        # each paragraph wrapped, the first flush, the rest indented two
        # spaces as the banners' paragraphs are
        lead = "" if n == 0 else "  "
        lines += ([""] if blank else []) + textwrap.wrap(
            lead + paragraph, 72, break_long_words=False,
            break_on_hyphens=False)
    text = banner("quat.h", lines, "2026.10.04", "2026.11.21")
    text += "\n" + QUAT_HEAD
    text += QUAT_KERNELS_PURE
    for sub, ks, kq, root, title, prose in QUAT_ROOT_FORMS:
        text += (QUAT_KERNELS_ROOT.replace("@SUB@", str(sub))
                 .replace("@TITLE@", title).replace("@PROSE@", prose)
                 .replace("@KS@", ks).replace("@KQ@", kq)
                 .replace("@SQRT@", root))
    text += QUAT_KERNELS_FUNCTIONS
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        forms = ""
        for f, rt in (("_c", ""), ("", "_RT")):
            forms += QUAT_TRIG.replace("@F@", f).replace("@RT@", rt)
        exps = ""
        for fn, doc, params, kargs, cextra, nextra, plain in QUAT_EXP_SPECS:
            exps += (QUAT_EXP.replace("@PLAIN_BODY@", plain.rstrip("\n"))
                     .replace("@FN@", fn).replace("@DOC@", doc)
                     .replace("@PARAMS@", params).replace("@KARGS@", kargs)
                     .replace("@CPLX_EXTRA@", cextra)
                     .replace("@NAN_EXTRA@", nextra))
        body = QUAT_FAMILY.replace("@TRIG_FORMS@", forms)
        body = body.replace("@EXP_FORMS@", exps)
        body = body.replace("@NARROW_D2@",
                            QUAT_NARROW_D2.get(suffix, "").rstrip("\n"))
        body = body.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        family = quat_d2(instantiate(body, suffix, ctype, prefix, prose),
                         suffix)
        # long double's algebra takes long double's pairs alone: its prose
        # says so where float's and double's names both pairs
        if suffix == "ld":
            for old, new in QUAT_LD_PROSE:
                assert family.count(old) == 1, old
                family = family.replace(old, new)
        text += family
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)",
                             QUAT_OVERLOADS)
    text += QUAT_TAIL
    return text


QUAT_NARROW_D2 = {"f": r"""// d_internal_quat_f_narrow_d2
//   _out = each of _p's _n double pairs rounded once to float, as
// d_internal_math_f_round_pair rounds a long double one: the conversion,
// and where it lands on a midpoint between two floats the low part's sign;
// a finite sum past float's range an infinity, but for the midpoint above
// the largest finite float with the sum below it.
D_MATH_FN void
d_internal_quat_f_narrow_d2(
    float*                                _out,
    const struct dj_internal_math_d_pair* _p,
    size_t                                _n
)
{
    size_t i = 0u;

    // each pair, rounded once
    for (i = 0u; i < _n; ++i)
    {
        const double hi    = _p[i].high;
        const double lo    = _p[i].low;
        const float  near  = (float)hi;
        const double rest  = hi - (double)near;
        float        value = near;

        // a finite sum the conversion took past the largest finite float
        if ( ( (near > FLT_MAX) ||
               (near < -FLT_MAX) ) &&
             (hi <= DBL_MAX) &&
             (hi >= -DBL_MAX) )
        {
            const double gap = (double)FLT_MAX -
                               (double)d_math_f_next_down(FLT_MAX);
            const double mid = (near > 0.0f) ?
                                   ((double)FLT_MAX + (gap / 2.0)) :
                                   (-(double)FLT_MAX - (gap / 2.0));

            // the midpoint above it, the sum below: the largest float
            if ( (hi == mid) &&
                 ( (near > 0.0f) ? (lo < 0.0) : (lo > 0.0) ) )
            {
                value = (near > 0.0f) ? FLT_MAX : -FLT_MAX;
            }
        }
        // hi rounded down to near, from the midpoint, the sum above it
        else if ( (rest > 0.0) &&
                  (lo > 0.0) )
        {
            const float up = d_math_f_next_up(near);

            if ((rest + rest) == ((double)up - (double)near))
            {
                value = up;
            }
        }
        // hi rounded up to near, from the midpoint, the sum below it
        else if ( (rest < 0.0) &&
                  (lo < 0.0) )
        {
            const float down = d_math_f_next_down(near);

            if ((rest + rest) == ((double)down - (double)near))
            {
                value = down;
            }
        }

        _out[i] = value;
    }

    return;
}
""", "d": r"""// d_internal_quat_d_narrow_d2
//   _out = each of _p's _n pairs' high part, the sum rounded once.
D_MATH_FN void
d_internal_quat_d_narrow_d2(
    double*                               _out,
    const struct dj_internal_math_d_pair* _p,
    size_t                                _n
)
{
    size_t i = 0u;

    // each pair's high part
    for (i = 0u; i < _n; ++i)
    {
        _out[i] = _p[i].high;
    }

    return;
}
"""}


QUAT_LD_PROSE = (
    ("// the sum of its four products as Dot2 sums them, rounded once: in "
     "double's\n// pairs where double is evaluated in its own type (section "
     "1.3), long\n// double's otherwise (section 1.2); where an operand has "
     "an infinite or NaN\n// part, the formula as written "
     "(d_quat_ld_multiply_fast).",
     "// the sum of its four products as Dot2 sums them in long double's "
     "pairs\n// (section 1.2), rounded once; where an operand has an "
     "infinite or NaN\n// part, the formula as written "
     "(d_quat_ld_multiply_fast)."),
    ("// once, in double's pairs or long double's as d_quat_ld_multiply "
     "takes\n// them.",
     "// once, in long double's pairs as d_quat_ld_multiply takes them."),
)


def quat_d2(text, suffix):
    """text with each block "#if @D2@ ... #else  // @D2@ ... #endif  // @D2@"
    made the test of double's evaluation for float and double, whose
    algebra takes double's pairs where double is evaluated in its own type;
    for long double its #else part alone (cplx_wide's way)."""
    keep = suffix in ("f", "d")
    out = []
    state = None
    for line in text.split("\n"):
        if line == "#if @D2@":
            state = "d2"
            if keep:
                out.append("#if !D_MATH_DBL_EVAL_WIDE")
            continue
        if line == "#else  // @D2@":
            state = "else"
            if keep:
                out.append("#else  // !D_MATH_DBL_EVAL_WIDE")
            continue
        if line == "#endif  // @D2@":
            state = None
            if keep:
                out.append("#endif  // !D_MATH_DBL_EVAL_WIDE")
            continue
        if (state == "d2") and (not keep):
            continue
        out.append(line)
    return "\n".join(out)
