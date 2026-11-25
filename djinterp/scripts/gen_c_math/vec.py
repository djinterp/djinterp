"""gen_c_math.vec - the text of vec.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (DATE, FAMILIES, banner, instantiate, overload_section,
                     re)


# -----------------------------------------------------------------------------
# vec.h
# -----------------------------------------------------------------------------

VEC_SHARED = r"""

//==============================================================================
// 1.  THE COMPENSATED SUMS' ARITHMETIC
//==============================================================================
// dot, sum and cross, and mat.h's products, trace and refinement, sum their
// terms as Ogita, Rump and Oishi's Dot2 and Sum2 do (S404): each product
// exact as a pair, by Dekker's product with Veltkamp's split, each sum by
// Knuth's two-sum, the errors summed apart and added last - as accurate as
// the sum computed in twice the precision and rounded once. They work in
// pairs of double where double is evaluated in its own type, as SSE
// evaluates it, for the float and double families, and in pairs of long
// double otherwise and for the long double family, as quat.h's products do
// (S276): where double is evaluated wider, as on a 32-bit x87, a split or a
// two-sum in double cannot be made exact in pure arithmetic. The long double
// pairs are elementary.h's (1.1); here are the double ones and the scaling.


// 1.1    In double
//------------------------------------------------------------------------------
#if !D_MATH_DBL_EVAL_WIDE

// d_internal_vec_d2_two_sum
//   _a + _b exactly, as high + low: Knuth's two-sum, which needs no order
// between the operands' magnitudes.
D_MATH_FN struct dj_internal_math_d_pair
d_internal_vec_d2_two_sum(
    double _a,
    double _b
)
{
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };
    const double                   high   = _a + _b;
    const double                   a_part = high - _b;
    const double                   b_part = high - a_part;

    result.high = high;
    result.low  = (_a - a_part) + (_b - b_part);

    return result;
}

// d_internal_vec_d2_product
//   _a * _b exactly, as high + low: the low part by a fused multiply-add
// where the target has one for double, and otherwise by Dekker's product,
// each factor split by Veltkamp's constant 2^ceil(P/2) + 1 (float_order.h);
// both exact, so both give the same bits (math_common.h, section 5). Each
// factor is 0 or within 2^(EMIN/2 + P) to 2^(EMAX/2 - P) in magnitude, as
// the kernels scale them, so that nothing overflows and the low part is
// exact.
D_MATH_FN struct dj_internal_math_d_pair
d_internal_vec_d2_product(
    double _a,
    double _b
)
{
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };

#if D_MATH_DBL_FMA
    if (D_MATH_RUN_TIME())
    {
        result.high = _a * _b;
        result.low  = D_MATH_DBL_FMA_OF(_a, _b, -result.high);

        return result;
    }
#endif  // D_MATH_DBL_FMA

    const double                   split  = d_internal_math_d_veltkamp();
    const double                   ta     = split * _a;
    const double                   a_hi   = ta - (ta - _a);
    const double                   a_lo   = _a - a_hi;
    const double                   tb     = split * _b;
    const double                   b_hi   = tb - (tb - _b);
    const double                   b_lo   = _b - b_hi;
    const double                   high   = _a * _b;

    result.high = high;
    result.low  = ( (((a_hi * b_hi) - high) + (a_hi * b_lo)) +
                    (a_lo * b_hi) ) +
                  (a_lo * b_lo);

    return result;
}

// d_internal_vec_d2_short
//   _a * _b as high + low where the product is exact in double, as a
// product of two floats is (24 and 24 bits of 53): the product and 0.
D_MATH_FN struct dj_internal_math_d_pair
d_internal_vec_d2_short(
    double _a,
    double _b
)
{
    struct dj_internal_math_d_pair result = { 0.0, 0.0 };

    result.high = _a * _b;

    return result;
}

// d_internal_vec_d2_scale
//   _x 2^_k, rounded once: in steps of at most 2^(EMAX - 1), so that no step
// overflows or underflows before the last, which rounds where the result is
// subnormal.
D_MATH_FN double
d_internal_vec_d2_scale(
    double _x,
    int    _k
)
{
    double value = _x;
    int    left  = _k;

    // whole steps up while more than one is left
    while (left > (DBL_MAX_EXP - 1))
    {
        value = value * d_internal_math_d_pow2(DBL_MAX_EXP - 1);
        left  = left - (DBL_MAX_EXP - 1);
    }

    // whole steps down while more than one is left
    while (left < (DBL_MIN_EXP - 1))
    {
        value = value * d_internal_math_d_pow2(DBL_MIN_EXP - 1);
        left  = left - (DBL_MIN_EXP - 1);
    }

    return value * d_internal_math_d_pow2(left);
}

#endif  // !D_MATH_DBL_EVAL_WIDE


// 1.2    In long double
//------------------------------------------------------------------------------
// The two-sum and Dekker's product are elementary.h's
// (d_internal_math_ld_two_sum, d_internal_math_ld_product_exact).

// d_internal_vec_ld_short
//   _a * _b as high + low where the product is exact in long double, as a
// product of two floats is in any long double of 48 bits or more: the
// product and 0.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_vec_ld_short(
    long double _a,
    long double _b
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = _a * _b;

    return result;
}

// d_internal_vec_ld_scale
//   _x 2^_k, rounded once, in steps as d_internal_vec_d2_scale takes them.
D_MATH_FN long double
d_internal_vec_ld_scale(
    long double _x,
    int         _k
)
{
    long double value = _x;
    int         left  = _k;

    // whole steps up while more than one is left
    while (left > (LDBL_MAX_EXP - 1))
    {
        value = value * d_internal_math_ld_pow2(LDBL_MAX_EXP - 1);
        left  = left - (LDBL_MAX_EXP - 1);
    }

    // whole steps down while more than one is left
    while (left < (LDBL_MIN_EXP - 1))
    {
        value = value * d_internal_math_ld_pow2(LDBL_MIN_EXP - 1);
        left  = left - (LDBL_MIN_EXP - 1);
    }

    return value * d_internal_math_ld_pow2(left);
}

"""
VEC_FAMILY_HELPERS = r"""// @SECTION@.2    Compensated sums
//------------------------------------------------------------------------------
// The kernels the products, the sums and mat.h's products, trace and
// refinement take (S404): each a sum c + a . (x + t) of strided operands,
// rounded once from Dot2's or Sum2's pair in the pairs section 1 says.
// Where an operand has a NaN or an infinity, the sum is the formula as
// written, as d_vec_@S@_dot_fast computes it, so that every non-finite case
// is the arithmetic's own.

// d_internal_vec_@S@_naive
//   _c + sum_i _a[i _sa] (_x[i _sx] + _t[i _st]) as written, the products
// negated where _minus and _t none but where _tailed: each product rounded, then
// added, in order - d_vec_@S@_dot_fast's formula, for operands with a NaN
// or an infinity.
D_MATH_FN @T@
d_internal_vec_@S@_naive(
    @T@        _c,
    bool       _minus,
    const @T@* _a,
    size_t     _sa,
    const @T@* _x,
    size_t     _sx,
    const @T@* _t,
    size_t     _st,
    bool       _tailed,
    size_t     _n
)
{
    @T@    acc = _c;
    size_t i   = 0u;

    // each product rounded, then added
    for (i = 0u; i < _n; ++i)
    {
        const @T@ x       = _minus ? -_x[i * _sx] : _x[i * _sx];
        const @T@ product = _a[i * _sa] * x;

        acc += product;

        // the tail's product after it
        if (_tailed)
        {
            const @T@ u    = _minus ? -_t[i * _st] : _t[i * _st];
            const @T@ tail = _a[i * _sa] * u;

            acc += tail;
        }
    }

    return acc;
}

// d_internal_vec_@S@_naive_sum
//   sum_i _a[i _sa] as written, in order from +0: d_vec_@S@_sum_fast's
// formula, for terms with a NaN or an infinity.
D_MATH_FN @T@
d_internal_vec_@S@_naive_sum(
    const @T@* _a,
    size_t     _sa,
    size_t     _n
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    // each term, in order
    for (i = 0u; i < _n; ++i)
    {
        acc += _a[i * _sa];
    }

    return acc;
}

// d_internal_vec_@S@_largest_of
//   the largest magnitude of the _n entries _v[i _stride], each finite: 0
// where each is 0.
D_MATH_FN @T@
d_internal_vec_@S@_largest_of(
    const @T@* _v,
    size_t     _stride,
    size_t     _n
)
{
    @T@    largest = (@T@)0;
    size_t i       = 0u;

    // each entry's magnitude
    for (i = 0u; i < _n; ++i)
    {
        const @T@ v = _v[i * _stride];
        const @T@ a = (v < (@T@)0) ? -v : v;

        if (a > largest)
        {
            largest = a;
        }
    }

    return largest;
}

// d_internal_vec_@S@_exponent_of
//   the binade E of a finite nonzero _x, 2^E <= |_x| < 2^(E + 1): a
// subnormal lifted into the normal range first.
D_MATH_FN int
d_internal_vec_@S@_exponent_of(
    @T@ _x
)
{
    const @T@ a    = (_x < (@T@)0) ? -_x : _x;
    const int lift = (a < @MIN@) ? @P@ : 0;
    const @T@ v    = (lift > 0) ? (a * d_internal_math_@S@_pow2(lift)) : a;

    return d_internal_math_@S@_binade_of(v).exponent - lift;
}

"""
VEC_WORK = r"""// d_internal_vec_@S@_@WS@_reach
//   of the _n entries _v[i _stride]: 2 where one is a NaN or an infinity;
// else 1 where one is not 0 and lies outside about 2^(EMIN/2 + P) to
// 2^(EMAX/2 - P) in @W@'s terms, where Dekker's products of two such entries
// neither overflow nor lose their low parts - its square, rounded, from
// 2^(EMIN + 2P + 1) to (1 - 2^-P) 2^(EMAX - 2P), bounds the range's own
// constants make with no power to build - else 0.
D_MATH_FN int
d_internal_vec_@S@_@WS@_reach(
    const @T@* _v,
    size_t     _stride,
    size_t     _n
)
{
    const @W@ low   = @WMIN@ * ((@W@)16 / (@WEPS@ * @WEPS@));
    const @W@ high  = (@WMAX@ * @WEPS@) * (@WEPS@ / (@W@)4);
    int       reach = 0;
    size_t    i     = 0u;

    // each entry, until a NaN or an infinity
    for (i = 0u; i < _n; ++i)
    {
        const @W@ v = (@W@)_v[i * _stride];
        const @W@ a = (v < (@W@)0) ? -v : v;

        // a NaN or an infinity: no sum to compensate
        if (!(a <= (@W@)@MAX@))
        {
            return 2;
        }

        // outside the range, but for 0
        if ( (a != (@W@)0) &&
             ( ((a * a) < low) ||
               ((a * a) > high) ) )
        {
            reach = 1;
        }
    }

    return reach;
}

// d_internal_vec_@S@_@WS@_sum_of
//   _c + sum_i _a[i _sa] (_x[i _sx] + _t[i _st]) as a pair of @W@, the
// products negated where _minus and _t none but where _tailed, every entry finite:
// _a's entries first times 2^-_ka, _x's and _t's times 2^-_kx and _c times
// 2^-(_ka + _kx), exactly but where an entry falls into the subnormals; each
// product exact (@PRODUCT@), the high parts summed by two-sums, the low parts
// and the two-sums' errors after, and the pair renormalized - Ogita, Rump
// and Oishi's Dot2, the sum as accurate as one in twice @W@'s precision
// rounded once. An exact zero is +0.
D_MATH_FN struct @WPAIR@
d_internal_vec_@S@_@WS@_sum_of(
    @T@        _c,
    bool       _minus,
    const @T@* _a,
    size_t     _sa,
    const @T@* _x,
    size_t     _sx,
    const @T@* _t,
    size_t     _st,
    bool       _tailed,
    size_t     _n,
    int        _ka,
    int        _kx
)
{
    struct @WPAIR@ result = { (@W@)0, (@W@)0 };
    const bool     scaled = ( (_ka != 0) ||
                              (_kx != 0) );
    const @W@      sign   = _minus ? (@W@)-1 : (@W@)1;
    @W@            a_up   = (@W@)1;
    @W@            a_down = (@W@)1;
    @W@            x_up   = (@W@)1;
    @W@            x_down = (@W@)1;
    @W@            high   = (@W@)_c;
    @W@            rest   = (@W@)0;
    size_t         i      = 0u;

    // the factors, each a normal power of 2, and _c scaled with the products
    if (scaled)
    {
        a_up   = d_internal_math_@WF@_pow2((-_ka) / 2);
        a_down = d_internal_math_@WF@_pow2((-_ka) - ((-_ka) / 2));
        x_up   = d_internal_math_@WF@_pow2((-_kx) / 2);
        x_down = d_internal_math_@WF@_pow2((-_kx) - ((-_kx) / 2));
        high   = d_internal_vec_@WS@_scale(high, -(_ka + _kx));
    }

    // each product exactly, its high part into the running sum
    for (i = 0u; i < _n; ++i)
    {
        const @W@ ae = (@W@)_a[i * _sa];
        const @W@ xe = sign * (@W@)_x[i * _sx];
        const @W@ a  = scaled ? ((ae * a_up) * a_down) : ae;
        const @W@ x  = scaled ? ((xe * x_up) * x_down) : xe;
        const struct @WPAIR@ p = @PRODUCT@(a, x);
        const struct @WPAIR@ s = @TWOSUM@(high, p.high);

        high = s.high;
        rest = rest + (s.low + p.low);

        // the tail's product after it
        if (_tailed)
        {
            const @W@ te = sign * (@W@)_t[i * _st];
            const @W@ u  = scaled ? ((te * x_up) * x_down) : te;
            const struct @WPAIR@ q = @PRODUCT@(a, u);
            const struct @WPAIR@ v = @TWOSUM@(high, q.high);

            high = v.high;
            rest = rest + (v.low + q.low);
        }
    }

    result = @TWOSUM@(high, rest);

    // an exact zero: +0
    if (result.high == (@W@)0)
    {
        result.high = (@W@)0;
        result.low  = (@W@)0;
    }

    return result;
}

@ROUND@
// d_internal_vec_@S@_@WS@_dot
//   _c + sum_i _a[i _sa] (_x[i _sx] + _t[i _st]), the products negated where
// _minus and _t none but where _tailed, rounded once to @T@ from Dot2's pair: as
// they are where every entry lies where Dekker's products are exact and _c
// within a quarter of @W@'s largest value; else each operand scaled, its
// largest entry to [1, 2), and the pair back - or _c itself, where the
// products' sum lies below half its rounding. Where an operand has a NaN or
// an infinity, the formula as written (d_internal_vec_@S@_naive).
D_MATH_FN @T@
d_internal_vec_@S@_@WS@_dot(
    @T@        _c,
    bool       _minus,
    const @T@* _a,
    size_t     _sa,
    const @T@* _x,
    size_t     _sx,
    const @T@* _t,
    size_t     _st,
    bool       _tailed,
    size_t     _n
)
{
    const int ra = d_internal_vec_@S@_@WS@_reach(_a, _sa, _n);
    const int rx = d_internal_vec_@S@_@WS@_reach(_x, _sx, _n);
    const int rt = d_internal_vec_@S@_@WS@_reach(_t, _st, _tailed ? _n : 0u);
    const @T@ ac = (_c < (@T@)0) ? -_c : _c;

    // a NaN or an infinity: the formula as written
    if ( (ra == 2) || (rx == 2) || (rt == 2) || (!(ac <= @MAX@)) )
    {
        return d_internal_vec_@S@_naive(_c, _minus, _a, _sa, _x, _sx, _t, _st,
                                        _tailed, _n);
    }

    // every entry in range: no scaling
    if ( (ra == 0) && (rx == 0) && (rt == 0) &&
         ((@W@)ac <= (@WMAX@ / (@W@)4)) )
    {
        return d_internal_vec_@S@_@WS@_round(
            d_internal_vec_@S@_@WS@_sum_of(_c, _minus, _a, _sa, _x, _sx, _t,
                                           _st, _tailed, _n, 0, 0),
            0);
    }

    {
        const @T@ la   = d_internal_vec_@S@_largest_of(_a, _sa, _n);
        const @T@ lx   = d_internal_vec_@S@_largest_of(_x, _sx, _n);
        const @T@ lt   = d_internal_vec_@S@_largest_of(_t, _st,
                                                   _tailed ? _n : 0u);
        const @T@ lu   = (lt > lx) ? lt : lx;
        int       ka   = 0;
        int       kx   = 0;
        int       bits = 0;
        size_t    left = _n;

        // a zero operand: each product 0, the sum _c's (+0 for either 0)
        if ( (la == (@T@)0) ||
             (lu == (@T@)0) )
        {
            return _c + (@T@)0;
        }

        ka = d_internal_vec_@S@_exponent_of(la);
        kx = d_internal_vec_@S@_exponent_of(lu);

        // the products' sum lies below 2^(ka + kx + 2 + bits)
        while (left > 0u)
        {
            ++bits;
            left /= 2u;
        }

        // _c far above it: _c itself, the products below half its rounding
        if ( (ac != (@T@)0) &&
             (d_internal_vec_@S@_exponent_of(ac) >
              (ka + kx + bits + @P@ + 3)) )
        {
            return _c;
        }

        return d_internal_vec_@S@_@WS@_round(
            d_internal_vec_@S@_@WS@_sum_of(_c, _minus, _a, _sa, _x, _sx, _t,
                                           _st, _tailed, _n, ka, kx),
            ka + kx);
    }
}

// d_internal_vec_@S@_@WS@_sum
//   sum_i _a[i _sa], rounded once to @T@ from Sum2's pair - Ogita, Rump and
// Oishi's: each term into the running sum by a two-sum, the errors summed
// apart and added last, the pair renormalized - as accurate as the sum in
// twice @W@'s precision, rounded once. Where twice _n times the largest
// term could pass @W@'s largest value, the terms are first scaled, the
// largest to [1, 2), exactly but where one falls into the subnormals, and the
// sum back. An exact zero is +0; where a term is a NaN or an infinity, the
// terms summed as written (d_internal_vec_@S@_naive_sum).
D_MATH_FN @T@
d_internal_vec_@S@_@WS@_sum(
    const @T@* _a,
    size_t     _sa,
    size_t     _n
)
{
    struct @WPAIR@ result  = { (@W@)0, (@W@)0 };
    @T@            largest = (@T@)0;
    int            k       = 0;
    @W@            high    = (@W@)0;
    @W@            rest    = (@W@)0;
    size_t         i       = 0u;

    // a NaN or an infinity: the terms as written
    if (d_internal_vec_@S@_@WS@_reach(_a, _sa, _n) == 2)
    {
        return d_internal_vec_@S@_naive_sum(_a, _sa, _n);
    }

    largest = d_internal_vec_@S@_largest_of(_a, _sa, _n);

    // each term 0: +0
    if (largest == (@T@)0)
    {
        return (@T@)0;
    }

    // a partial sum that could overflow: the terms scaled
    if ((@W@)largest > (@WMAX@ / ((@W@)2 * (@W@)_n)))
    {
        k = d_internal_vec_@S@_exponent_of(largest);
    }

    {
        @W@ up   = (@W@)1;
        @W@ down = (@W@)1;

        // the factors, each a normal power of 2, where the terms are scaled
        if (k != 0)
        {
            up   = d_internal_math_@WF@_pow2((-k) / 2);
            down = d_internal_math_@WF@_pow2((-k) - ((-k) / 2));
        }

        // each term into the running sum, its error apart
        for (i = 0u; i < _n; ++i)
        {
            const @W@ e    = (@W@)_a[i * _sa];
            const @W@ term = (k != 0) ? ((e * up) * down) : e;
            const struct @WPAIR@ s = @TWOSUM@(high, term);

            high = s.high;
            rest = rest + s.low;
        }
    }

    result = @TWOSUM@(high, rest);

    // an exact zero: +0
    if (result.high == (@W@)0)
    {
        return (@T@)0;
    }

    return d_internal_vec_@S@_@WS@_round(result, k);
}

@ADD_PAIR@
"""
VEC_ROUND_SAME = r"""// d_internal_vec_@S@_@WS@_round
//   (_p.high + _p.low) 2^_k rounded once to @T@, _p a renormalized pair -
// its high part the sum rounded: the high part scaled, exact where the
// result is normal and an infinity past the range, as the sum's rounding
// makes it; in the subnormals the scaling rounds again, and where the
// scaled high part lay exactly between two subnormals, the low part's sign
// decides.
D_MATH_FN @T@
d_internal_vec_@S@_@WS@_round(
    struct @WPAIR@ _p,
    int            _k
)
{
    const @T@ value = (_k == 0) ? _p.high
                                : d_internal_vec_@WS@_scale(_p.high, _k);
    const @T@ a     = (value < (@T@)0) ? -value : value;
    const @T@ tiny  = @MIN@ * @EPS@;

    // scaled up or not at all, exact, normal or past the range: the scaled
    // high part, the sum's rounding
    if ( (_k >= 0) ||
         (_p.low == (@T@)0) ||
         (!(a < @MIN@)) )
    {
        return value;
    }

    {
        const @T@ back = d_internal_vec_@WS@_scale(value, -_k);
        const @T@ rest = _p.high - back;
        const @T@ gap  = d_internal_vec_@WS@_scale(tiny, -_k);

        // rounded down from the midpoint, and the sum lies above it
        if ( ((rest + rest) == gap) &&
             (_p.low > (@T@)0) )
        {
            return value + tiny;
        }

        // rounded up from the midpoint, and the sum lies below it
        if ( ((rest + rest) == -gap) &&
             (_p.low < (@T@)0) )
        {
            return value - tiny;
        }
    }

    return value;
}
"""
VEC_ROUND_WIDE = r"""// d_internal_vec_@S@_@WS@_round
//   (_p.high + _p.low) 2^_k rounded once to @T@, _p a renormalized pair of
// @W@: scaled, exactly in @W@'s wider range, then rounded as elementary.h's
// d_internal_math_@S@_round_pair rounds a kernel's pair - the high part
// converted, and where it lies exactly between two values of @T@ the low
// part's sign deciding. Each such midpoint is a value of @W@, so the high
// part, the sum rounded to @W@ rather than to long double, lies on one
// only where it is the sum's own rounding or the sum is the midpoint.
D_MATH_FN @T@
d_internal_vec_@S@_@WS@_round(
    struct @WPAIR@ _p,
    int            _k
)
{
    const @W@ high = (_k == 0) ? _p.high
                               : d_internal_vec_@WS@_scale(_p.high, _k);
    const @W@ low  = (_k == 0) ? _p.low
                               : d_internal_vec_@WS@_scale(_p.low, _k);

    return d_internal_math_@S@_round_pair((long double)high,
                                          (long double)low);
}
"""
VEC_ADD_PAIR_SAME = r"""// d_internal_vec_@S@_@WS@_add_pair
//   *_x + *_tail += _d, the pair renormalized: its high part the sum rounded
// once and its low part the rest, by two-sums - exact but for the rest's
// own rounding.
D_MATH_FN void
d_internal_vec_@S@_@WS@_add_pair(
    @T@* _x,
    @T@* _tail,
    @T@  _d
)
{
    const struct @WPAIR@ s = @TWOSUM@(*_x, _d);
    const struct @WPAIR@ p = @TWOSUM@(s.high, s.low + *_tail);

    *_x    = p.high;
    *_tail = p.low;

    return;
}
"""
VEC_ADD_PAIR_WIDE = r"""// d_internal_vec_@S@_@WS@_add_pair
//   *_x + *_tail += _d in @W@'s pairs, then *_x the sum rounded once to @T@
// (d_internal_vec_@S@_@WS@_round) and *_tail the rest, rounded to @T@.
D_MATH_FN void
d_internal_vec_@S@_@WS@_add_pair(
    @T@* _x,
    @T@* _tail,
    @T@  _d
)
{
    const struct @WPAIR@ s    = @TWOSUM@((@W@)*_x, (@W@)_d);
    const struct @WPAIR@ p    = @TWOSUM@(s.high, s.low + (@W@)*_tail);
    const @T@            high = d_internal_vec_@S@_@WS@_round(p, 0);

    *_x    = high;
    *_tail = (@T@)((p.high - (@W@)high) + p.low);

    return;
}
"""
VEC_FAMILY_KERNELS = r"""
// d_internal_vec_@S@_reach
//   of the _n entries _v[i _stride]: 2 where one is a NaN or an infinity,
// else 1 where one leaves the range where Dekker's products of two such
// entries are exact in the pairs the family sums in, else 0.
D_MATH_FN int
d_internal_vec_@S@_reach(
    const @T@* _v,
    size_t     _stride,
    size_t     _n
)
{
@DISPATCH{@
    return d_internal_vec_@S@_@WS@_reach(_v, _stride, _n);
@}DISPATCH@
}

// d_internal_vec_@S@_dot2
//   _c + sum_i _a[i _sa] (_x[i _sx] + _t[i _st]), the products negated where
// _minus and _t none but where _tailed, rounded once from Dot2's pair; the formula
// as written where an operand has a NaN or an infinity.
D_MATH_FN @T@
d_internal_vec_@S@_dot2(
    @T@        _c,
    bool       _minus,
    const @T@* _a,
    size_t     _sa,
    const @T@* _x,
    size_t     _sx,
    const @T@* _t,
    size_t     _st,
    bool       _tailed,
    size_t     _n
)
{
@DISPATCH{@
    return d_internal_vec_@S@_@WS@_dot(_c, _minus, _a, _sa, _x, _sx, _t, _st, _tailed, _n);
@}DISPATCH@
}

// d_internal_vec_@S@_dot2_direct
//   d_internal_vec_@S@_dot2 for operands its caller has found in range -
// each of _a's, _x's and _t's entries of reach 0 and _c 0 - as a product's
// entries are where both factors were found so once: no test, no scaling.
D_MATH_FN @T@
d_internal_vec_@S@_dot2_direct(
    bool       _minus,
    const @T@* _a,
    size_t     _sa,
    const @T@* _x,
    size_t     _sx,
    size_t     _n
)
{
@DISPATCH{@
    const struct @WPAIR@ sum = d_internal_vec_@S@_@WS@_sum_of((@T@)0, _minus, _a, _sa, _x, _sx, _x, _sx, false, _n, 0, 0);

    return d_internal_vec_@S@_@WS@_round(sum, 0);
@}DISPATCH@
}

// d_internal_vec_@S@_sum2
//   sum_i _a[i _sa], rounded once from Sum2's pair; the terms as written
// where one is a NaN or an infinity.
D_MATH_FN @T@
d_internal_vec_@S@_sum2(
    const @T@* _a,
    size_t     _sa,
    size_t     _n
)
{
@DISPATCH{@
    return d_internal_vec_@S@_@WS@_sum(_a, _sa, _n);
@}DISPATCH@
}

// d_internal_vec_@S@_add_pair
//   *_x + *_tail += _d, the pair renormalized, its high part the sum rounded
// once: how mat.h's refinement carries a solution with its tail.
D_MATH_FN void
d_internal_vec_@S@_add_pair(
    @T@* _x,
    @T@* _tail,
    @T@  _d
)
{
@DISPATCH{@
    d_internal_vec_@S@_@WS@_add_pair(_x, _tail, _d);
@}DISPATCH@

    return;
}

// dj_internal_vec_@S@_acc
//   struct: Dot2's running sum for a caller whose terms are not strided
// arrays of the family's type - a complex matrix's parts (cmat.h, linalg's
// complex kernels) - in the pairs the family sums in: high, the products'
// high parts summed by two-sums, and rest, their errors and low parts.
struct dj_internal_vec_@S@_acc
{
@DISPATCH{@
    @W@ high;
    @W@ rest;
@}DISPATCH@
};

// d_internal_vec_@S@_acc_of
//   a running sum of _c alone.
D_MATH_FN struct dj_internal_vec_@S@_acc
d_internal_vec_@S@_acc_of(
    @T@ _c
)
{
    struct dj_internal_vec_@S@_acc acc = { 0, 0 };

    acc.high = _c;

    return acc;
}

// d_internal_vec_@S@_acc_add
//   *_acc plus _a _x, exactly: Dot2's step, each factor of reach 0
// (d_internal_vec_@S@_reach_of) - the caller scales it first where it is
// not.
D_MATH_FN void
d_internal_vec_@S@_acc_add(
    struct dj_internal_vec_@S@_acc* _acc,
    @T@                             _a,
    @T@                             _x
)
{
@DISPATCH{@
    const struct @WPAIR@ p = @PRODUCT@(_a, _x);
    const struct @WPAIR@ s = @TWOSUM@(_acc->high, p.high);

    _acc->high = s.high;
    _acc->rest = _acc->rest + (s.low + p.low);
@}DISPATCH@

    return;
}

// d_internal_vec_@S@_acc_add_term
//   *_acc plus _t, exactly: Sum2's step.
D_MATH_FN void
d_internal_vec_@S@_acc_add_term(
    struct dj_internal_vec_@S@_acc* _acc,
    @T@                             _t
)
{
@DISPATCH{@
    const struct @WPAIR@ s = @TWOSUM@(_acc->high, _t);

    _acc->high = s.high;
    _acc->rest = _acc->rest + s.low;
@}DISPATCH@

    return;
}

// d_internal_vec_@S@_acc_round
//   _acc's sum times 2^_k, rounded once to @T@ (the round of the pairs the
// family sums in): +0 for an exact zero.
D_MATH_FN @T@
d_internal_vec_@S@_acc_round(
    struct dj_internal_vec_@S@_acc _acc,
    int                            _k
)
{
@DISPATCH{@
    const struct @WPAIR@ sum = @TWOSUM@(_acc.high, _acc.rest);

    // an exact zero: +0
    if (sum.high == 0)
    {
        return (@T@)0;
    }

    return d_internal_vec_@S@_@WS@_round(sum, _k);
@}DISPATCH@
}

// d_internal_vec_@S@_acc_scale
//   the power by which Sum2 scales _n finite terms, the largest _largest in
// magnitude, as the family's sums scale them: _largest's binade where twice
// _n times it could pass the largest value of the pairs the family sums in,
// 0 where it could not. A caller of d_internal_vec_@S@_acc_add_term scales
// each term by it (d_internal_vec_@S@_scaled) and rounds the sum back by it.
D_MATH_FN int
d_internal_vec_@S@_acc_scale(
    @T@    _largest,
    size_t _n
)
{
@DISPATCH{@
    // a partial sum that could overflow: the terms scaled
    if ((@W@)_largest > (@WMAX@ / ((@W@)2 * (@W@)_n)))
    {
        return d_internal_vec_@S@_exponent_of(_largest);
    }
@}DISPATCH@

    return 0;
}

// d_internal_vec_@S@_reach_of
//   _v's reach, as d_internal_vec_@S@_reach gives an operand's: 2 for a NaN
// or an infinity, 1 outside the range where Dekker's products are exact, 0
// within it or for 0.
D_MATH_FN int
d_internal_vec_@S@_reach_of(
    @T@ _v
)
{
    return d_internal_vec_@S@_reach(&_v, 1u, 1u);
}

// d_internal_vec_@S@_scaled
//   _v 2^-_k, _k a finite value's binade or its negation, in two steps of
// normal powers of 2: exact but where the result falls into the
// subnormals.
D_MATH_FN @T@
d_internal_vec_@S@_scaled(
    @T@ _v,
    int _k
)
{
    const @T@ up   = d_internal_math_@S@_pow2((-_k) / 2);
    const @T@ down = d_internal_math_@S@_pow2((-_k) - ((-_k) / 2));

    return (_v * up) * down;
}

// d_internal_vec_@S@_dot2_scaled
//   sum_i (_a[i _sa] 2^-_ka) (_x[i _sx] 2^-_kx), each entry finite, rounded
// once from Dot2's pair: the scaled operands' dot, not scaled back, as the
// projection takes it at the range's ends.
D_MATH_FN @T@
d_internal_vec_@S@_dot2_scaled(
    const @T@* _a,
    size_t     _sa,
    const @T@* _x,
    size_t     _sx,
    size_t     _n,
    int        _ka,
    int        _kx
)
{
@DISPATCH{@
    const struct @WPAIR@ sum = d_internal_vec_@S@_@WS@_sum_of((@T@)0, false, _a, _sa, _x, _sx, _x, _sx, false, _n, _ka, _kx);

    return d_internal_vec_@S@_@WS@_round(sum, 0);
@}DISPATCH@
}

"""


def wrap_call(line):
    """a call's line wrapped at 80 columns, its arguments packed under the
    first: how the dispatches below are written."""
    if len(line) <= 80:
        return line
    open_ = line.index("(")
    close = line.rindex(")")
    inner = line[open_ + 1:close]
    args, depth, cur = [], 0, ""
    for ch in inner:
        if (ch == ",") and (depth == 0):
            args.append(cur.strip())
            cur = ""
            continue
        depth += (ch == "(") - (ch == ")")
        cur += ch
    args.append(cur.strip())
    col = open_ + 1
    out, row = [], line[:col]
    for i, a in enumerate(args):
        piece = a + ("," if i < len(args) - 1 else line[close:])
        if (row.strip() and (len(row) + len(piece) + 1 > 80)
                and (len(row) > col)):
            out.append(row.rstrip())
            row = " " * col + piece
        else:
            row += ("" if len(row) == col else " ") + piece
    out.append(row)
    return "\n".join(out)


def tidy80(text, comments=True):
    """the compensated sums' text, its family's names filled in, kept to 80
    columns: each comment paragraph reflowed (a function's name line kept,
    its first line of text indented two more), and each declaration whose
    initializer would pass the edge broken after its '=' and the call
    wrapped (wrap_call)."""
    import re
    lines = text.split("\n")
    out = []
    i = 0
    comment = re.compile(r"^(\s*)// ?(.*)$")
    while i < len(lines):
        m = comment.match(lines[i])
        rule = lines[i].lstrip().startswith(("//==", "//--"))
        if (not comments) or (m is None) or rule or (lines[i].strip() == "//"):
            out.append(lines[i])
            i += 1
            continue
        indent = m.group(1)
        block = []
        while (i < len(lines)):
            mm = comment.match(lines[i])
            if ((mm is None) or (mm.group(1) != indent) or
                    lines[i].lstrip().startswith(("//==", "//--")) or
                    (lines[i].strip() == "//")):
                break
            block.append(lines[i])
            i += 1
        name = None
        body = block
        if re.match(r"^\s*// [A-Za-z_][A-Za-z_0-9]*$", block[0]) and \
                (len(block) > 1) and block[1].startswith(indent + "//   "):
            name, body = block[0], block[1:]
        words = " ".join(comment.match(b).group(2).strip() for b in body).split()
        first = indent + ("//   " if name else "// ")
        rest = indent + "// "
        if name:
            out.append(name)
        row = first
        for w in words:
            if (len(row) + len(w) + (0 if row in (first, rest) else 1) > 80) \
                    and (row not in (first, rest)):
                out.append(row)
                row = rest
            row += ("" if row in (first, rest) else " ") + w
        out.append(row)
    # declarations past the edge: broken after '=', the call wrapped
    result = []
    decl = re.compile(r"^(\s*)((?:const )?(?:struct )?[A-Za-z_][\w ]*?[\w*]) +(\w+)(\s*)= (.+)$")
    for line in out:
        if (len(line) > 80) and not line.lstrip().startswith("//"):
            m = decl.match(line)
            if m and m.group(5).rstrip().endswith(";"):
                ind = m.group(1)
                result.append("%s%s %s =" % (ind, m.group(2), m.group(3)))
                result.append(wrap_call(ind + "    " + m.group(5)))
                continue
            if re.match(r"^\s*(return )?[A-Za-z_]\w*\(.*\);$", line):
                result.append(wrap_call(line))
                continue
        result.append(line)
    return "\n".join(result)


def vec_work(suffix):
    """the family's sums in pairs: double's where double is evaluated in its
    own type and long double's otherwise, long double's for long double."""
    def one(ws):
        wtype = {"d2": "double", "ld": "long double"}[ws]
        wpre = {"d2": "DBL", "ld": "LDBL"}[ws]
        same = ((suffix, ws) in (("d", "d2"), ("ld", "ld")))
        body = VEC_WORK.replace("@ROUND@",
                                VEC_ROUND_SAME if same else VEC_ROUND_WIDE)
        body = body.replace("@ADD_PAIR@",
                            VEC_ADD_PAIR_SAME if same else VEC_ADD_PAIR_WIDE)
        if suffix == "f":
            product = "d_internal_vec_%s_short" % ws
        else:
            product = {"d2": "d_internal_vec_d2_product",
                       "ld": "d_internal_math_ld_product_exact"}[ws]
        for key, value in (
            ("@WPAIR@", {"d2": "dj_internal_math_d_pair",
                         "ld": "dj_internal_math_ld_pair"}[ws]),
            ("@PRODUCT@", product),
            ("@TWOSUM@", {"d2": "d_internal_vec_d2_two_sum",
                          "ld": "d_internal_math_ld_two_sum"}[ws]),
            ("@WS@", ws),
            ("@WF@", {"d2": "d", "ld": "ld"}[ws]),
            ("@WP@", wpre + "_MANT_DIG"),
            ("@WEMIN@", wpre + "_MIN_EXP"),
            ("@WEMAX@", wpre + "_MAX_EXP"),
            ("@WMAX@", wpre + "_MAX"),
            ("@WMIN@", wpre + "_MIN"),
            ("@WEPS@", wpre + "_EPSILON"),
            ("@W@", wtype),
        ):
            body = body.replace(key, value)
        return body
    if suffix == "ld":
        return one("ld")
    return ("#if !D_MATH_DBL_EVAL_WIDE\n\n" + one("d2") +
            "\n#else  // !D_MATH_DBL_EVAL_WIDE\n\n" + one("ld") +
            "\n#endif  // !D_MATH_DBL_EVAL_WIDE\n")


def vec_kernels(suffix, ctype):
    """the family's kernels, each dispatching to the pairs the target takes:
    @DISPATCH{@ ... @}DISPATCH@ holds the statements, one to a line."""
    text = VEC_FAMILY_KERNELS
    while "@DISPATCH{@" in text:
        i = text.index("@DISPATCH{@\n")
        j = text.index("@}DISPATCH@\n", i)
        stmts = text[i + len("@DISPATCH{@\n"):j].splitlines()

        def fill(ws):
            pair = {"d2": "dj_internal_math_d_pair",
                    "ld": "dj_internal_math_ld_pair"}[ws]
            wtype = {"d2": "double", "ld": "long double"}[ws]
            if suffix == "f":
                product = "d_internal_vec_%s_short" % ws
            else:
                product = {"d2": "d_internal_vec_d2_product",
                           "ld": "d_internal_math_ld_product_exact"}[ws]
            twosum = {"d2": "d_internal_vec_d2_two_sum",
                      "ld": "d_internal_math_ld_two_sum"}[ws]
            return "\n".join(
                s.replace("@WS@", ws).replace("@WPAIR@", pair)
                 .replace("@PRODUCT@", product).replace("@TWOSUM@", twosum)
                 .replace("@WMAX@", {"d2": "DBL_MAX", "ld": "LDBL_MAX"}[ws])
                 .replace("@W@", wtype)
                 .replace("@S@", suffix).replace("@T@", ctype)
                for s in stmts) + "\n"
        if suffix == "ld":
            block = fill("ld")
        else:
            block = ("#if !D_MATH_DBL_EVAL_WIDE\n" + fill("d2") +
                     "#else  // !D_MATH_DBL_EVAL_WIDE\n" + fill("ld") +
                     "#endif  // !D_MATH_DBL_EVAL_WIDE\n")
        text = text[:i] + block + text[j + len("@}DISPATCH@\n"):]
    return text


VEC_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================
// _count components each; _out may be _a or _b.


// @SECTION@.1    Element-wise arithmetic
//------------------------------------------------------------------------------

D_MATH_FN void
d_vec_@S@_fill(
    @T@*   _out,
    size_t _count,
    @T@    _value
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _value;
    }

    return;
}

D_MATH_FN void
d_vec_@S@_add(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] + _b[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_sub(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] - _b[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_scale(
    @T@*       _out,
    const @T@* _a,
    @T@        _s,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] * _s;
    }

    return;
}

D_MATH_FN void
d_vec_@S@_divide(
    @T@*       _out,
    const @T@* _a,
    @T@        _s,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] / _s;
    }

    return;
}

D_MATH_FN void
d_vec_@S@_negate(
    @T@*       _out,
    const @T@* _a,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = -_a[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_hadamard(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        _out[i] = _a[i] * _b[i];
    }

    return;
}

D_MATH_FN void
d_vec_@S@_lerp(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b,
    @T@        _t,
    size_t     _count
)
{
    const @T@ keep = (@T@)1 - _t;
    size_t    i    = 0u;

    // each product rounded apart, then summed: a(1 - t) + bt
    for (i = 0u; i < _count; ++i)
    {
        const @T@ from = _a[i] * keep;
        const @T@ to   = _b[i] * _t;

        _out[i] = from + to;
    }

    return;
}

@COMPENSATED@// @SECTION@.3    Products and reductions
//------------------------------------------------------------------------------
// dot, cross and sum are compensated (S404): each a sum rounded once from
// Dot2's or Sum2's pair (@SECTION@.2), as accurate as the sum computed in
// twice the precision and rounded once, so correctly rounded but where its
// terms cancel past twice the precision or it lies within that of a
// midpoint. The _fast forms are the formulas as written, each product
// rounded before it is summed, in order, so that no compiler can fuse the
// two and a result is the same on every target: faster, for double some ten
// times, and their error unbounded where the terms cancel. product, min and
// max are as written.

// d_vec_@S@_dot
//   the sum of the _count products _a[i] _b[i], rounded once from Dot2's
// pair; where an operand has a NaN or an infinity, d_vec_@S@_dot_fast's.
D_MATH_FN @T@
d_vec_@S@_dot(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    return d_internal_vec_@S@_dot2((@T@)0, false, _a, 1u, _b, 1u, _b, 1u,
                                   false, _count);
}

// d_vec_@S@_dot_fast
//   the sum of the _count products as written: each product rounded, then
// added, in order from +0.
D_MATH_FN @T@
d_vec_@S@_dot_fast(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ product = _a[i] * _b[i];

        acc += product;
    }

    return acc;
}

// d_internal_vec_@S@_difference
//   _p _q - _r _s rounded once from Dot2's pair of the two products; where
// the two products are equal, the formula as written, whose 0 then keeps
// the sign the arithmetic gives it. Where a factor is a NaN or an infinity,
// Dot2's kernel gives the formula's value (d_internal_vec_@S@_naive).
D_MATH_FN @T@
d_internal_vec_@S@_difference(
    @T@ _p,
    @T@ _q,
    @T@ _r,
    @T@ _s
)
{
    const @T@ left  = _p * _q;
    const @T@ right = _r * _s;
    const @T@ a[2]  = { _p, _r };
    const @T@ b[2]  = { _q, -_s };
    const @T@ value = d_internal_vec_@S@_dot2((@T@)0, false, a, 1u, b, 1u, b, 1u, false, 2u);

    // the products equal and so their difference 0: the formula's, its 0's
    // sign the arithmetic's
    if ( (value == (@T@)0) &&
         (left == right) )
    {
        return left - right;
    }

    return value;
}

// d_vec_@S@_cross
//   _out = _a x _b, each part the difference of its two products rounded
// once from Dot2's pair (d_internal_vec_@S@_difference). _out may be _a or
// _b.
D_MATH_FN void
d_vec_@S@_cross(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // all three from the inputs before any output is written
    const @T@ x = d_internal_vec_@S@_difference(_a[1], _b[2], _a[2], _b[1]);
    const @T@ y = d_internal_vec_@S@_difference(_a[2], _b[0], _a[0], _b[2]);
    const @T@ z = d_internal_vec_@S@_difference(_a[0], _b[1], _a[1], _b[0]);

    _out[0] = x;
    _out[1] = y;
    _out[2] = z;

    return;
}

// d_vec_@S@_cross_fast
//   _out = _a x _b as written: each part's two products rounded, then
// subtracted. _out may be _a or _b.
D_MATH_FN void
d_vec_@S@_cross_fast(
    @T@*       _out,
    const @T@* _a,
    const @T@* _b
)
{
    // all three from the inputs before any output is written
    const @T@ x1 = _a[1] * _b[2];
    const @T@ x2 = _a[2] * _b[1];
    const @T@ y1 = _a[2] * _b[0];
    const @T@ y2 = _a[0] * _b[2];
    const @T@ z1 = _a[0] * _b[1];
    const @T@ z2 = _a[1] * _b[0];

    _out[0] = x1 - x2;
    _out[1] = y1 - y2;
    _out[2] = z1 - z2;

    return;
}

// d_vec_@S@_sum
//   the sum of the _count terms, rounded once from Sum2's pair; where a term
// is a NaN or an infinity, d_vec_@S@_sum_fast's.
D_MATH_FN @T@
d_vec_@S@_sum(
    const @T@* _a,
    size_t     _count
)
{
    return d_internal_vec_@S@_sum2(_a, 1u, _count);
}

// d_vec_@S@_sum_fast
//   the sum of the _count terms as written, in order from +0.
D_MATH_FN @T@
d_vec_@S@_sum_fast(
    const @T@* _a,
    size_t     _count
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        acc += _a[i];
    }

    return acc;
}

D_MATH_FN @T@
d_vec_@S@_product(
    const @T@* _a,
    size_t     _count
)
{
    @T@    acc = (@T@)1;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        acc *= _a[i];
    }

    return acc;
}

D_MATH_FN @T@
d_vec_@S@_min(
    const @T@* _a,
    size_t     _count
)
{
    @T@    m = _a[0];
    size_t i = 1u;

    for (i = 1u; i < _count; ++i)
    {
        if (_a[i] < m)
        {
            m = _a[i];
        }
    }

    return m;
}

D_MATH_FN @T@
d_vec_@S@_max(
    const @T@* _a,
    size_t     _count
)
{
    @T@    m = _a[0];
    size_t i = 1u;

    for (i = 1u; i < _count; ++i)
    {
        if (_a[i] > m)
        {
            m = _a[i];
        }
    }

    return m;
}

// @SECTION@.4    Norms and distances
//------------------------------------------------------------------------------
// Each root comes in two forms, the same value bit for bit (elementary.h's
// two square roots are both correctly rounded): the _c form a constant
// expression from C++14, the plain one the C library's root. A norm is the
// root of the sum of squares wherever that root is in range - at most the
// family's largest value, and at least the root of its least normal over its
// epsilon, where no square overflowed or lost its precision to underflow -
// and elsewhere the norm of the entries scaled exactly by a power of two to
// a largest magnitude in [1, 2), scaled back, as LAPACK's dnrm2 and Julia's
// norm scale (S256): a norm the family holds is never an infinity, nor 0 for
// a vector that is not zero, and a norm in range keeps its bits. distance,
// normalize and cos_angle scale as norm does.

// d_internal_vec_@S@_largest
//   the largest magnitude of the _count entries _a[i] - _b[i], or _a[i]
// where _b is NULL: 0 for the zero vector, and -1 where an entry is a NaN or
// an infinity, for which nothing is scaled.
D_MATH_FN @T@
d_internal_vec_@S@_largest(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    @T@    largest = (@T@)0;
    size_t i       = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ e = (_b != NULL) ? (_a[i] - _b[i]) : _a[i];
        const @T@ v = (e < (@T@)0) ? -e : e;

        // a NaN or an infinity
        if (!(v <= @MAX@))
        {
            return (@T@)-1;
        }

        if (v > largest)
        {
            largest = v;
        }
    }

    return largest;
}

// d_internal_vec_@S@_power_of
//   the power of two that brings a positive finite _largest into [1, 2) -
// its binade - as *_power, times *_pre, 2^P where _largest is subnormal and
// is lifted first, else 1: an entry scaled by it, times *_pre and then over
// *_power, is exact (but where it underflows, below the largest's rounding).
D_MATH_FN void
d_internal_vec_@S@_power_of(
    @T@  _largest,
    @T@* _pre,
    @T@* _power
)
{
    @T@ lifted = _largest;

    *_pre = (@T@)1;

    // a subnormal largest: lifted into the normal range first
    if (_largest < @MIN@)
    {
        *_pre  = d_internal_math_@S@_pow2(@P@);
        lifted = _largest * *_pre;
    }

    {
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(lifted);

        *_power = b.power;
    }

    return;
}

// d_internal_vec_@S@_scaled_entry
//   entry _i of the vector - _a[_i] - _b[_i], or _a[_i] where _b is NULL -
// times _pre, then over _power.
D_MATH_FN @T@
d_internal_vec_@S@_scaled_entry(
    const @T@* _a,
    const @T@* _b,
    size_t     _i,
    @T@        _pre,
    @T@        _power
)
{
    const @T@ e      = (_b != NULL) ? (_a[_i] - _b[_i]) : _a[_i];
    const @T@ lifted = e * _pre;

    return lifted / _power;
}

// d_internal_vec_@S@_scaled_sum
//   the sum of the squares of the entries scaled by _pre and _power, as
// d_vec_@S@_dot_fast sums: each square rounded, then added.
D_MATH_FN @T@
d_internal_vec_@S@_scaled_sum(
    const @T@* _a,
    const @T@* _b,
    size_t     _count,
    @T@        _pre,
    @T@        _power
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ v      = d_internal_vec_@S@_scaled_entry(_a, _b, i, _pre,
                                                           _power);
        const @T@ square = v * v;

        acc += square;
    }

    return acc;
}

// d_vec_@S@_norm_squared
//   the sum of the squares as written (d_vec_@S@_dot_fast's): a sum of
// terms that cannot cancel, within (n - 1) u of its value relatively, and
// the bits the norms keep where they are in range (S256, S405).
D_MATH_FN @T@
d_vec_@S@_norm_squared(
    const @T@* _a,
    size_t     _count
)
{
    return d_vec_@S@_dot_fast(_a, _a, _count);
}

// d_internal_vec_@S@_in_range_c
//   whether _norm, the root of a sum of squares, is in the range where no
// square overflowed or lost its precision to underflow: at most the family's
// largest value, and at least the root of its least normal over its epsilon.
D_MATH_FN bool
d_internal_vec_@S@_in_range_c(
    @T@ _norm
)
{
    const @T@ small = d_math_@S@_sqrt_c(@MIN@ / @EPS@);

    return ( (_norm <= @MAX@) &&
             (!(_norm < small)) );
}

// d_internal_vec_@S@_rescued_c
//   _norm, the root of the sum of squares of the entries _a[i] - _b[i] (or
// _a[i] where _b is NULL), where it is in range; elsewhere the norm of the
// entries scaled by a power of two to a largest magnitude in [1, 2), scaled
// back; _norm itself for the zero vector or where an entry is a NaN or an
// infinity.
D_MATH_FN @T@
d_internal_vec_@S@_rescued_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count,
    @T@        _norm
)
{
    const @T@ largest = d_internal_vec_@S@_largest(_a, _b, _count);
    @T@       pre     = (@T@)1;
    @T@       power   = (@T@)1;

    // in range, or nothing to scale
    if ( (d_internal_vec_@S@_in_range_c(_norm)) ||
         (!(largest > (@T@)0)) )
    {
        return _norm;
    }

    d_internal_vec_@S@_power_of(largest, &pre, &power);

    {
        const @T@ sum      = d_internal_vec_@S@_scaled_sum(_a, _b, _count,
                                                           pre, power);
        const @T@ root     = d_math_@S@_sqrt_c(sum);
        const @T@ restored = root * power;

        return restored / pre;
    }
}

D_MATH_FN @T@
d_vec_@S@_norm_c(
    const @T@* _a,
    size_t     _count
)
{
    const @T@ norm = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_a, _count));

    return d_internal_vec_@S@_rescued_c(_a, NULL, _count, norm);
}

// d_internal_vec_@S@_in_range
//   whether _norm, the root of a sum of squares, is in the range where no
// square overflowed or lost its precision to underflow: at most the family's
// largest value, and at least the root of its least normal over its epsilon.
D_MATH_FN_RT bool
d_internal_vec_@S@_in_range(
    @T@ _norm
)
{
    const @T@ small = d_math_@S@_sqrt(@MIN@ / @EPS@);

    return ( (_norm <= @MAX@) &&
             (!(_norm < small)) );
}

// d_internal_vec_@S@_rescued
//   _norm, the root of the sum of squares of the entries _a[i] - _b[i] (or
// _a[i] where _b is NULL), where it is in range; elsewhere the norm of the
// entries scaled by a power of two to a largest magnitude in [1, 2), scaled
// back; _norm itself for the zero vector or where an entry is a NaN or an
// infinity.
D_MATH_FN_RT @T@
d_internal_vec_@S@_rescued(
    const @T@* _a,
    const @T@* _b,
    size_t     _count,
    @T@        _norm
)
{
    const @T@ largest = d_internal_vec_@S@_largest(_a, _b, _count);
    @T@       pre     = (@T@)1;
    @T@       power   = (@T@)1;

    // in range, or nothing to scale
    if ( (d_internal_vec_@S@_in_range(_norm)) ||
         (!(largest > (@T@)0)) )
    {
        return _norm;
    }

    d_internal_vec_@S@_power_of(largest, &pre, &power);

    {
        const @T@ sum      = d_internal_vec_@S@_scaled_sum(_a, _b, _count,
                                                           pre, power);
        const @T@ root     = d_math_@S@_sqrt(sum);
        const @T@ restored = root * power;

        return restored / pre;
    }
}

D_MATH_FN_RT @T@
d_vec_@S@_norm(
    const @T@* _a,
    size_t     _count
)
{
    const @T@ norm = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_a, _count));

    return d_internal_vec_@S@_rescued(_a, NULL, _count, norm);
}

D_MATH_FN @T@
d_vec_@S@_distance_squared(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    @T@    acc = (@T@)0;
    size_t i   = 0u;

    // the difference's own square, as |a - b|^2 is
    for (i = 0u; i < _count; ++i)
    {
        const @T@ d      = _a[i] - _b[i];
        const @T@ square = d * d;

        acc += square;
    }

    return acc;
}

D_MATH_FN @T@
d_vec_@S@_distance_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ norm =
        d_math_@S@_sqrt_c(d_vec_@S@_distance_squared(_a, _b, _count));

    return d_internal_vec_@S@_rescued_c(_a, _b, _count, norm);
}

D_MATH_FN_RT @T@
d_vec_@S@_distance(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ norm =
        d_math_@S@_sqrt(d_vec_@S@_distance_squared(_a, _b, _count));

    return d_internal_vec_@S@_rescued(_a, _b, _count, norm);
}

D_MATH_FN void
d_vec_@S@_normalize_c(
    @T@*       _out,
    const @T@* _a,
    size_t     _count
)
{
    const @T@ n = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_a, _count));
    @T@       largest = (@T@)0;

    // in range: over the norm, as ever
    if (d_internal_vec_@S@_in_range_c(n))
    {
        d_vec_@S@_divide(_out, _a, n, _count);

        return;
    }

    largest = d_internal_vec_@S@_largest(_a, NULL, _count);

    // the zero vector stays zero
    if (largest == (@T@)0)
    {
        d_vec_@S@_fill(_out, _count, (@T@)0);
    }
    // a NaN or an infinity: over the norm, as ever
    else if (!(largest > (@T@)0))
    {
        d_vec_@S@_divide(_out, _a, n, _count);
    }
    // the vector scaled by a power of two first, which leaves its direction
    else
    {
        @T@    pre   = (@T@)1;
        @T@    power = (@T@)1;
        @T@    root  = (@T@)0;
        size_t i     = 0u;

        d_internal_vec_@S@_power_of(largest, &pre, &power);
        root = d_math_@S@_sqrt_c(
                   d_internal_vec_@S@_scaled_sum(_a, NULL, _count, pre,
                                                 power));

        for (i = 0u; i < _count; ++i)
        {
            _out[i] = d_internal_vec_@S@_scaled_entry(_a, NULL, i, pre,
                                                      power) / root;
        }
    }

    return;
}

D_MATH_FN_RT void
d_vec_@S@_normalize(
    @T@*       _out,
    const @T@* _a,
    size_t     _count
)
{
    const @T@ n = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_a, _count));
    @T@       largest = (@T@)0;

    // in range: over the norm, as ever
    if (d_internal_vec_@S@_in_range(n))
    {
        d_vec_@S@_divide(_out, _a, n, _count);

        return;
    }

    largest = d_internal_vec_@S@_largest(_a, NULL, _count);

    // the zero vector stays zero
    if (largest == (@T@)0)
    {
        d_vec_@S@_fill(_out, _count, (@T@)0);
    }
    // a NaN or an infinity: over the norm, as ever
    else if (!(largest > (@T@)0))
    {
        d_vec_@S@_divide(_out, _a, n, _count);
    }
    // the vector scaled by a power of two first, which leaves its direction
    else
    {
        @T@    pre   = (@T@)1;
        @T@    power = (@T@)1;
        @T@    root  = (@T@)0;
        size_t i     = 0u;

        d_internal_vec_@S@_power_of(largest, &pre, &power);
        root = d_math_@S@_sqrt(
                   d_internal_vec_@S@_scaled_sum(_a, NULL, _count, pre,
                                                 power));

        for (i = 0u; i < _count; ++i)
        {
            _out[i] = d_internal_vec_@S@_scaled_entry(_a, NULL, i, pre,
                                                      power) / root;
        }
    }

    return;
}

// @SECTION@.5    Projection and angle
//------------------------------------------------------------------------------
// project is _a's component along _onto, reject what is left; the zero
// vector projects to zero. Both divide the compensated dot by the squared
// norm where those and their ratio lie in range, and scale their operands
// elsewhere (S301). cos_angle is 0 when either vector is zero, and scaled as
// the norms are where their product would leave the range; angle clamps it
// into [-1, 1] first, so rounding past 1 gives 0, not NaN.

// d_internal_vec_@S@_along
//   _out = _a's projection onto _onto, or where _reject what is left of _a;
// _out may be either input. In range - _onto's squared norm d from MIN/EPS
// to MAX, the compensated dot p 0 or its ratio to d from MIN/EPS to MAX -
// each component p/d times _onto's, as before (S301: a result in range
// keeps its bits); so too for the zero vector, or an operand with a NaN or
// an infinity, which give zeros or the arithmetic's own values. Elsewhere,
// both operands nonzero and finite: each scaled, its largest entry to [1,
// 2), the scaled dot p' (Dot2) over the scaled squared norm d', and each
// component p'/d' times _onto's scaled entry, scaled back by _a's power -
// exact but for the arithmetic's own roundings and the last step's into the
// subnormals, so that a projection in range is computed in range wherever
// its operands lie.
D_MATH_FN void
d_internal_vec_@S@_along(
    @T@*       _out,
    const @T@* _a,
    const @T@* _onto,
    size_t     _count,
    bool       _reject
)
{
    const @T@ d        = d_vec_@S@_norm_squared(_onto, _count);
    const @T@ p        = d_vec_@S@_dot(_a, _onto, _count);
    const @T@ ap       = (p < (@T@)0) ? -p : p;
    const @T@ lo       = @MIN@ / @EPS@;
    const @T@ la       = d_internal_vec_@S@_largest(_a, NULL, _count);
    const @T@ lb       = d_internal_vec_@S@_largest(_onto, NULL, _count);
    const bool in_range =
        ( (d >= lo) && (d <= @MAX@) &&
          ( (ap == (@T@)0) ||
            ( (ap >= lo) && (ap <= @MAX@) && ((ap / @MAX@) <= d) &&
              ((ap / d) >= lo) ) ) );
    size_t    i        = 0u;

    // in range, zero or not finite: the formula as written
    if ( (in_range) ||
         (!(la > (@T@)0)) ||
         (!(lb > (@T@)0)) )
    {
        const @T@ ratio = (d > (@T@)0) ? (p / d) : (@T@)0;

        for (i = 0u; i < _count; ++i)
        {
            const @T@ along = _onto[i] * ratio;

            // onto the zero vector there is nothing to project
            if (_reject)
            {
                _out[i] = _a[i] - along;
            }
            else
            {
                _out[i] = (d > (@T@)0) ? along : (@T@)0;
            }
        }

        return;
    }

    {
        const int ka = d_internal_vec_@S@_exponent_of(la);
        const int kb = d_internal_vec_@S@_exponent_of(lb);
        const @T@ ps = d_internal_vec_@S@_dot2_scaled(_a, 1u, _onto, 1u,
                                                      _count, ka, kb);
        @T@       ds = (@T@)0;
        @T@       rs = (@T@)0;

        // the scaled squared norm, summed as norm_squared sums
        for (i = 0u; i < _count; ++i)
        {
            const @T@ b      = d_internal_vec_@S@_scaled(_onto[i], kb);
            const @T@ square = b * b;

            ds += square;
        }

        rs = ps / ds;

        // each component from the inputs before it is written
        for (i = 0u; i < _count; ++i)
        {
            const @T@ b     = d_internal_vec_@S@_scaled(_onto[i], kb);
            const @T@ along = d_internal_vec_@S@_scaled(rs * b, -ka);

            _out[i] = _reject ? (_a[i] - along) : along;
        }
    }

    return;
}

D_MATH_FN void
d_vec_@S@_project(
    @T@*       _out,
    const @T@* _a,
    const @T@* _onto,
    size_t     _count
)
{
    d_internal_vec_@S@_along(_out, _a, _onto, _count, false);

    return;
}

D_MATH_FN void
d_vec_@S@_reject(
    @T@*       _out,
    const @T@* _a,
    const @T@* _from,
    size_t     _count
)
{
    d_internal_vec_@S@_along(_out, _a, _from, _count, true);

    return;
}

D_MATH_FN @T@
d_vec_@S@_cos_angle_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ na = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_a, _count));
    const @T@ nb = d_math_@S@_sqrt_c(d_vec_@S@_norm_squared(_b, _count));
    const @T@ d  = na * nb;
    @T@       la = (@T@)0;
    @T@       lb = (@T@)0;

    // in range: the dot product over the norms' product, as ever
    if ( (d_internal_vec_@S@_in_range_c(na)) &&
         (d_internal_vec_@S@_in_range_c(nb)) &&
         (d <= @MAX@) )
    {
        return d_vec_@S@_dot(_a, _b, _count) / d;
    }

    la = d_internal_vec_@S@_largest(_a, NULL, _count);
    lb = d_internal_vec_@S@_largest(_b, NULL, _count);

    // a NaN or an infinity: as ever
    if ( (la < (@T@)0) ||
         (lb < (@T@)0) )
    {
        return (d > (@T@)0) ? (d_vec_@S@_dot(_a, _b, _count) / d) : (@T@)0;
    }

    // a zero vector: 0
    if ( (la == (@T@)0) ||
         (lb == (@T@)0) )
    {
        return (@T@)0;
    }

    // each vector scaled by a power of two, which leaves the cosine
    {
        @T@    pa  = (@T@)1;
        @T@    wa  = (@T@)1;
        @T@    pb  = (@T@)1;
        @T@    wb  = (@T@)1;
        @T@    dot = (@T@)0;
        size_t i   = 0u;

        d_internal_vec_@S@_power_of(la, &pa, &wa);
        d_internal_vec_@S@_power_of(lb, &pb, &wb);

        for (i = 0u; i < _count; ++i)
        {
            const @T@ x       = d_internal_vec_@S@_scaled_entry(_a, NULL, i,
                                                                pa, wa);
            const @T@ y       = d_internal_vec_@S@_scaled_entry(_b, NULL, i,
                                                                pb, wb);
            const @T@ product = x * y;

            dot += product;
        }

        {
            const @T@ sa = d_math_@S@_sqrt_c(
                               d_internal_vec_@S@_scaled_sum(_a, NULL, _count,
                                                             pa, wa));
            const @T@ sb = d_math_@S@_sqrt_c(
                               d_internal_vec_@S@_scaled_sum(_b, NULL, _count,
                                                             pb, wb));
            const @T@ ds = sa * sb;

            return dot / ds;
        }
    }
}

D_MATH_FN_RT @T@
d_vec_@S@_cos_angle(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ na = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_a, _count));
    const @T@ nb = d_math_@S@_sqrt(d_vec_@S@_norm_squared(_b, _count));
    const @T@ d  = na * nb;
    @T@       la = (@T@)0;
    @T@       lb = (@T@)0;

    // in range: the dot product over the norms' product, as ever
    if ( (d_internal_vec_@S@_in_range(na)) &&
         (d_internal_vec_@S@_in_range(nb)) &&
         (d <= @MAX@) )
    {
        return d_vec_@S@_dot(_a, _b, _count) / d;
    }

    la = d_internal_vec_@S@_largest(_a, NULL, _count);
    lb = d_internal_vec_@S@_largest(_b, NULL, _count);

    // a NaN or an infinity: as ever
    if ( (la < (@T@)0) ||
         (lb < (@T@)0) )
    {
        return (d > (@T@)0) ? (d_vec_@S@_dot(_a, _b, _count) / d) : (@T@)0;
    }

    // a zero vector: 0
    if ( (la == (@T@)0) ||
         (lb == (@T@)0) )
    {
        return (@T@)0;
    }

    // each vector scaled by a power of two, which leaves the cosine
    {
        @T@    pa  = (@T@)1;
        @T@    wa  = (@T@)1;
        @T@    pb  = (@T@)1;
        @T@    wb  = (@T@)1;
        @T@    dot = (@T@)0;
        size_t i   = 0u;

        d_internal_vec_@S@_power_of(la, &pa, &wa);
        d_internal_vec_@S@_power_of(lb, &pb, &wb);

        for (i = 0u; i < _count; ++i)
        {
            const @T@ x       = d_internal_vec_@S@_scaled_entry(_a, NULL, i,
                                                                pa, wa);
            const @T@ y       = d_internal_vec_@S@_scaled_entry(_b, NULL, i,
                                                                pb, wb);
            const @T@ product = x * y;

            dot += product;
        }

        {
            const @T@ sa = d_math_@S@_sqrt(
                               d_internal_vec_@S@_scaled_sum(_a, NULL, _count,
                                                             pa, wa));
            const @T@ sb = d_math_@S@_sqrt(
                               d_internal_vec_@S@_scaled_sum(_b, NULL, _count,
                                                             pb, wb));
            const @T@ ds = sa * sb;

            return dot / ds;
        }
    }
}

D_MATH_FN @T@
d_vec_@S@_angle_c(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ c = d_vec_@S@_cos_angle_c(_a, _b, _count);

    return d_math_@S@_acos_c( (c > (@T@)1)  ? (@T@)1  :
                              (c < (@T@)-1) ? (@T@)-1 : c );
}

D_MATH_FN_RT @T@
d_vec_@S@_angle(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    const @T@ c = d_vec_@S@_cos_angle(_a, _b, _count);

    return d_math_@S@_acos( (c > (@T@)1)  ? (@T@)1  :
                            (c < (@T@)-1) ? (@T@)-1 : c );
}

// @SECTION@.6    Comparison
//------------------------------------------------------------------------------
// equal is component-wise ==; equals allows each component _tolerance.

D_MATH_FN bool
d_vec_@S@_equal(
    const @T@* _a,
    const @T@* _b,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        if (!(_a[i] == _b[i]))
        {
            return false;
        }
    }

    return true;
}

D_MATH_FN bool
d_vec_@S@_equals(
    const @T@* _a,
    const @T@* _b,
    @T@        _tolerance,
    size_t     _count
)
{
    size_t i = 0u;

    for (i = 0u; i < _count; ++i)
    {
        const @T@ d = _a[i] - _b[i];

        if (((d < (@T@)0) ? -d : d) > _tolerance)
        {
            return false;
        }
    }

    return true;
}
"""

VEC_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE COMPENSATED SUMS' ARITHMETIC
    --------------------------------
    1.  In double
    2.  In long double
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Element-wise arithmetic
    2.  Compensated sums
    3.  Products and reductions
    4.  Norms and distances
    5.  Projection and angle
    6.  Comparison
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Element-wise arithmetic
    2.  Compensated sums
    3.  Products and reductions
    4.  Norms and distances
    5.  Projection and angle
    6.  Comparison
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Element-wise arithmetic
    2.  Compensated sums
    3.  Products and reductions
    4.  Norms and distances
    5.  Projection and angle
    6.  Comparison
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_VEC_H
#define DJINTERP_C_MATH_VEC_H 1

// std
#include <float.h>           // DBL_MAX_EXP ... LDBL_MIN_EXP: the scaling
#include <stddef.h>          // size_t
// djinterp
#include "./elementary.h"    // d_math_*_sqrt, _sqrt_c, _acos, _acos_c,
                             // the long double pairs, *_round_pair
#include "./float_order.h"   // d_internal_math_*_binade_of,
                             // d_internal_math_*_pow2: the scaling
#include "./math_common.h"   // D_MATH_FN
"""

VEC_TAIL = r"""

#endif  // DJINTERP_C_MATH_VEC_H
"""

_A = ("const @T@*", "_a")
_B = ("const @T@*", "_b")
_O = ("@T@*", "_out")
_N = ("size_t", "_count")
VEC_OVERLOADS = [
    ("D_MATH_FN", "void", "d_vec_fill", [_O, _N, ("@T@", "_value")]),
    ("D_MATH_FN", "void", "d_vec_add", [_O, _A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_sub", [_O, _A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_scale", [_O, _A, ("@T@", "_s"), _N]),
    ("D_MATH_FN", "void", "d_vec_divide", [_O, _A, ("@T@", "_s"), _N]),
    ("D_MATH_FN", "void", "d_vec_negate", [_O, _A, _N]),
    ("D_MATH_FN", "void", "d_vec_hadamard", [_O, _A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_lerp", [_O, _A, _B, ("@T@", "_t"), _N]),
    ("D_MATH_FN", "@T@", "d_vec_dot", [_A, _B, _N]),
    ("D_MATH_FN", "@T@", "d_vec_dot_fast", [_A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_cross", [_O, _A, _B]),
    ("D_MATH_FN", "void", "d_vec_cross_fast", [_O, _A, _B]),
    ("D_MATH_FN", "@T@", "d_vec_sum", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_sum_fast", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_product", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_min", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_max", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_norm_squared", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_norm_c", [_A, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_norm", [_A, _N]),
    ("D_MATH_FN", "@T@", "d_vec_distance_squared", [_A, _B, _N]),
    ("D_MATH_FN", "@T@", "d_vec_distance_c", [_A, _B, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_distance", [_A, _B, _N]),
    ("D_MATH_FN", "void", "d_vec_normalize_c", [_O, _A, _N]),
    ("D_MATH_FN_RT", "void", "d_vec_normalize", [_O, _A, _N]),
    ("D_MATH_FN", "void", "d_vec_project", [_O, _A, ("const @T@*", "_onto"), _N]),
    ("D_MATH_FN", "void", "d_vec_reject", [_O, _A, ("const @T@*", "_from"), _N]),
    ("D_MATH_FN", "@T@", "d_vec_cos_angle_c", [_A, _B, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_cos_angle", [_A, _B, _N]),
    ("D_MATH_FN", "@T@", "d_vec_angle_c", [_A, _B, _N]),
    ("D_MATH_FN_RT", "@T@", "d_vec_angle", [_A, _B, _N]),
    ("D_MATH_FN", "bool", "d_vec_equal", [_A, _B, _N]),
    ("D_MATH_FN", "bool", "d_vec_equals", [_A, _B, ("@T@", "_tolerance"), _N]),
    # the compensated kernels' accumulator, for the generic path's complex
    # sums (linalg_common.hpp), which sum each part as the C core does
    ("D_MATH_FN", "struct dj_internal_vec_@S@_acc", "d_internal_vec_acc_of",
     [("@T@", "_c")]),
    ("D_MATH_FN", "void", "d_internal_vec_acc_add",
     [("struct dj_internal_vec_@S@_acc*", "_acc"), ("@T@", "_a"),
      ("@T@", "_x")]),
    ("D_MATH_FN", "void", "d_internal_vec_acc_add_term",
     [("struct dj_internal_vec_@S@_acc*", "_acc"), ("@T@", "_t")]),
    ("D_MATH_FN", "@T@", "d_internal_vec_acc_round",
     [("struct dj_internal_vec_@S@_acc", "_acc"), ("int", "_k")]),
    ("D_MATH_FN", "int", "d_internal_vec_acc_scale",
     [("@T@", "_largest"), ("size_t", "_n")]),
    ("D_MATH_FN", "int", "d_internal_vec_reach_of", [("@T@", "_v")]),
    ("D_MATH_FN", "@T@", "d_internal_vec_scaled",
     [("@T@", "_v"), ("int", "_k")]),
    ("D_MATH_FN", "int", "d_internal_vec_exponent_of", [("@T@", "_x")]),
    ("D_MATH_FN", "void", "d_internal_vec_add_pair",
     [("@T@*", "_x"), ("@T@*", "_tail"), ("@T@", "_d")]),
]


def vec():
    lines = [
        "Fixed-size vectors: the C core of math/linear_algebra/vector.hpp, for",
        "float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A vector here is an array of _count components the caller owns, as",
        "BLAS takes them; the C++ face's storage is that array. Each kernel is a",
        "loop the face's constant dimension unrolls, a constant expression from",
        "C++14 but for the plain root forms, which call the C library. A norm",
        "is scaled by a power of two where its sum of squares would leave the",
        "range, and keeps its bits where it does not (S256). dot, cross and sum",
        "are compensated - Ogita, Rump and Oishi's Dot2 and Sum2, as accurate as",
        "the sum computed in twice the precision and rounded once - and their",
        "_fast forms are the formulas as written (S404). project and reject scale",
        "their operands where the squared norm or the ratio would leave the range",
        "(S301, S409). Dot2's running pair, an accumulator, serves the callers",
        "whose terms are not arrays of one family, cmat.h's and linalg's complex",
        "sums (S410, S411).",
    ]
    text = banner("vec.h", lines, DATE, "2026.11.21")
    text += "\n" + VEC_HEAD + VEC_SHARED
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = VEC_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        compensated = (VEC_FAMILY_HELPERS + vec_work(suffix) +
                       vec_kernels(suffix, ctype))
        compensated = instantiate(compensated.replace("@SECTION@", str(i + 2)),
                                  suffix, ctype, prefix, prose)
        body = body.replace("@COMPENSATED@", tidy80(compensated))
        text += tidy80(instantiate(body, suffix, ctype, prefix, prose),
                       comments=False)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", VEC_OVERLOADS)
    text += VEC_TAIL
    return text
