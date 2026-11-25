"""gen_c_math.cplx_text - the text of cplx.h, its first half (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""


# -----------------------------------------------------------------------------
# cplx.h
# -----------------------------------------------------------------------------
# Complex numbers (S265 to S270): a struct per family whose one member is an
# array of the two parts, so that it has the layout of T[2] that C99's
# _Complex T and C++'s std::complex<T> share; the arithmetic once per family,
# pure, with real, imag, conj and proj, which are exact; and C99's other 18
# functions twice, as every real function here is - the _c form on one long
# double kernel of pure arithmetic, rounded once to the family, and the plain
# form on a kernel of each family's own type on the C library's real
# functions. One kernel text serves both: @W@ is its working
# type, @KS@ its names' suffix and @R@ the suffix of the real functions it
# calls.

CPLX_TYPES = r"""
//==============================================================================
// 1.  THE TYPES
//==============================================================================
// One struct per family, of one member: the two parts, real first, as an
// array, so that the struct has the layout of T[2] - no padding lies between
// an array's elements, as it may between a struct's members - which C99's
// _Complex T (6.2.5) and C++'s std::complex<T> ([complex.numbers]) both
// promise. A value of either converts to one of these by a copy of its bytes.
// No typedef hides the struct (the C guide's rule): it is spelled struct
// dj_cplx_d.


// 1.1    The complex types
//------------------------------------------------------------------------------
@STRUCTS@
"""

CPLX_STRUCT = r"""// 1.1.@N@
// dj_cplx_@S@
//   struct: a complex number of @NAME@ parts: part[0] the real part,
// part[1] the imaginary.
struct dj_cplx_@S@
{
    @T@ part[2];
};

D_STATIC_ASSERT( (sizeof(struct dj_cplx_@S@) == (2 * sizeof(@T@))),
                 "struct dj_cplx_@S@ has the layout of @T@[2]" );
"""

# the arithmetic and its helpers, once per family: pure, so one form
CPLX_ARITH = r"""
// @SECTION@.@SUB@    The @NAME@ family (_@S@)
//------------------------------------------------------------------------------
// The helpers first - an exact product and sum, a power-of-2 scaling that
// never overflows on its way, the sign of a zero - then the operations. A
// product's parts are each an exact product less or plus another, by
// Dekker's product and two-sum, rounded once (Kahan's way, its error within
// 2 ulps of each part); a quotient multiplies by the conjugate over the
// squared modulus, each sum so formed, the denominator's low part kept. The
// operands are first scaled by powers of 2 so that no square or product
// overflows or loses its low part to underflow, and the result scaled back
// once. A zero part takes the sign the textbook formula gives it, a
// quotient's as a product's (S382), and an infinity C99's Annex G's
// recovery (G.5.1, its examples), so that an infinite operand gives an
// infinity where the formula's terms are NaN.

// d_internal_cplx_@S@_make
//   the complex number _x + _y i.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_make(
    @T@ _x,
    @T@ _y
)
{
    struct dj_cplx_@S@ result = { { (@T@)0, (@T@)0 } };

    result.part[0] = _x;
    result.part[1] = _y;

    return result;
}

// d_internal_cplx_@S@_is_inf
//   whether _x is an infinity of either sign.
D_MATH_FN bool
d_internal_cplx_@S@_is_inf(
    @T@ _x
)
{
    return ( (_x > @MAX@) ||
             (_x < -@MAX@) );
}

// d_internal_cplx_@S@_copysign
//   |_x| with the sign of _s, a zero's and a NaN's sign bit among them as
// tangent.h's negative reads them.
D_MATH_FN @T@
d_internal_cplx_@S@_copysign(
    @T@ _x,
    @T@ _s
)
{
    const @T@ magnitude = d_internal_math_@S@_negative(_x) ? -_x : _x;

    return d_internal_math_@S@_negative(_s) ? -magnitude : magnitude;
}

// d_internal_cplx_@S@_abs
//   |_x| by comparison, a constant expression, and +0 for either zero, as a
// magnitude is: elementary.h's own leaves a -0 as it is, which a part built
// of a magnitude would carry.
D_MATH_FN @T@
d_internal_cplx_@S@_abs(
    @T@ _x
)
{
    return (_x < (@T@)0) ? -_x : (_x + (@T@)0);
}

// d_internal_cplx_@S@_two_sum
//   _a + _b exactly, as high + low: Knuth's two-sum.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_cplx_@S@_two_sum(
    @T@ _a,
    @T@ _b
)
{
    struct dj_internal_math_@S@_pair result = { (@T@)0, (@T@)0 };
    const @T@ high   = _a + _b;
    const @T@ a_part = high - _b;
    const @T@ b_part = high - a_part;

    result.high = high;
    result.low  = (_a - a_part) + (_b - b_part);

    return result;
}

// d_internal_cplx_@S@_product
//   _a * _b exactly, as high + low: the low part by a fused multiply-add
// where the target has one for @NAME@, and otherwise by Dekker's product,
// each factor split by Veltkamp's constant (float_order.h); both exact, so
// both give the same bits (math_common.h, section 5). The factors are within
// 2^(EMAX/2 - P) in magnitude and their product, where it is not 0, at least
// 2^(EMIN + 2P), as the scalings below keep them, so that nothing overflows
// and the low part is exact.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_cplx_@S@_product(
    @T@ _a,
    @T@ _b
)
{
    struct dj_internal_math_@S@_pair result = { (@T@)0, (@T@)0 };

#if D_MATH_@PFX@_FMA
    if (D_MATH_RUN_TIME())
    {
        result.high = _a * _b;
        result.low  = D_MATH_@PFX@_FMA_OF(_a, _b, -result.high);

        return result;
    }
#endif  // D_MATH_@PFX@_FMA

    const @T@ split = d_internal_math_@S@_veltkamp();
    const @T@ ta    = split * _a;
    const @T@ a_hi  = ta - (ta - _a);
    const @T@ a_lo  = _a - a_hi;
    const @T@ tb    = split * _b;
    const @T@ b_hi  = tb - (tb - _b);
    const @T@ b_lo  = _b - b_hi;
    const @T@ high  = _a * _b;

    result.high = high;
    result.low  = ( (((a_hi * b_hi) - high) + (a_hi * b_lo)) +
                    (a_lo * b_hi) ) +
                  (a_lo * b_lo);

    return result;
}

#if @WIDE@
// d_internal_cplx_@S@_dot_wide
//   _a _b + _c _d as a long double pair, high the sum rounded to long double,
// where the target evaluates @T@ wider than its type, as a 32-bit x87 does:
// there Dekker's split and the two-sum, which need each operation rounded to
// @T@, cannot be made to have it in pure arithmetic - a value may keep the
// register's bits or lose them as the compiler spills it - so the products
// are taken exactly in long double, whose operations the target rounds in
// their own type, and their sum as one more pair.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_cplx_@S@_dot_wide(
    @T@ _a,
    @T@ _b,
    @T@ _c,
    @T@ _d
)
{
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact((long double)_a, (long double)_b);
    const struct dj_internal_math_ld_pair q =
        d_internal_math_ld_product_exact((long double)_c, (long double)_d);
    const struct dj_internal_math_ld_pair s =
        d_internal_math_ld_two_sum(p.high, q.high);
    struct dj_internal_math_ld_pair t =
        d_internal_math_ld_two_sum(s.high, s.low + (p.low + q.low));

    // an exact zero: the formula's own, with its sign
    if (t.high == 0.0L)
    {
        t.high = (long double)((_a * _b) + (_c * _d));
    }

    return t;
}

// d_internal_cplx_@S@_over_wide
//   (_u.high + _u.low) / (_n.high + _n.low), _n.high in [1, 8), rounded once
// to @T@, as d_internal_cplx_@S@_over takes it, in long double: a zero q
// itself, of its sign (S382).
D_MATH_FN @T@
d_internal_cplx_@S@_over_wide(
    struct dj_internal_math_ld_pair _u,
    struct dj_internal_math_ld_pair _n
)
{
    const long double q0    = _u.high / _n.high;
    const long double least =
        LDBL_MIN * d_internal_math_ld_pow2(LDBL_MANT_DIG + 4);

    // a zero: itself, its sign kept
    if (q0 == 0.0L)
    {
        return (@T@)q0;
    }

    // far below what @T@ holds: the quotient as it is
    if ( (q0 < least) &&
         (q0 > -least) )
    {
        return (@T@)(q0 - (q0 * (_n.low / _n.high)));
    }

    {
        const struct dj_internal_math_ld_pair t =
            d_internal_math_ld_product_exact(q0, _n.high);
        const long double head = (_u.high - t.high) - t.low;
        const long double r    = (head + _u.low) - (q0 * _n.low);
        const struct dj_internal_math_ld_pair v =
            d_internal_math_ld_two_sum(q0, r / _n.high);

        return d_internal_math_@S@_round_pair(v.high, v.low);
    }
}

#endif  // @WIDE@
// d_internal_cplx_@S@_dot_pair
//   _a _b + _c _d as high + low, high the sum rounded once from its exact
// terms: the two products exact, their high parts summed exactly, and the
// rest below. Where the exact sum is 0 the textbook formula's zero is taken,
// so that its sign is IEEE's.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_cplx_@S@_dot_pair(
    @T@ _a,
    @T@ _b,
    @T@ _c,
    @T@ _d
)
{
    const struct dj_internal_math_@S@_pair p =
        d_internal_cplx_@S@_product(_a, _b);
    const struct dj_internal_math_@S@_pair q =
        d_internal_cplx_@S@_product(_c, _d);
    const struct dj_internal_math_@S@_pair s =
        d_internal_cplx_@S@_two_sum(p.high, q.high);
    struct dj_internal_math_@S@_pair t =
        d_internal_cplx_@S@_two_sum(s.high, s.low + (p.low + q.low));

    // an exact zero: the formula's own, with its sign
    if (t.high == (@T@)0)
    {
        t.high = (_a * _b) + (_c * _d);
    }

    return t;
}

// d_internal_cplx_@S@_dot
//   _a _b + _c _d, rounded once from its exact terms, as the pair's high
// part (d_internal_cplx_@S@_dot_pair); where a target evaluates float and
// double wider than their types, as a 32-bit x87 does, the long double pair
// of d_internal_cplx_@S@_dot_wide rounded once.
D_MATH_FN @T@
d_internal_cplx_@S@_dot(
    @T@ _a,
    @T@ _b,
    @T@ _c,
    @T@ _d
)
{
#if @WIDE@
    const struct dj_internal_math_ld_pair t =
        d_internal_cplx_@S@_dot_wide(_a, _b, _c, _d);

    return d_internal_math_@S@_round_pair(t.high, t.low);
#else  // @WIDE@
    return d_internal_cplx_@S@_dot_pair(_a, _b, _c, _d).high;
#endif  // @WIDE@
}

// d_internal_cplx_@S@_over
//   (_u.high + _u.low) / (_n.high + _n.low), _n.high in [1, 8): the quotient
// q of the high parts, then the residual u - q n formed exactly of Dekker's
// product of q and n's high part, and q plus the residual over n, rounded
// once, so that the quotient is within a step of the correctly rounded one.
// A zero is q itself, the numerator's formula's zero over n, of its sign
// (S382); a quotient too near the subnormals for Dekker's product is q less
// q n_lo / n.
D_MATH_FN @T@
d_internal_cplx_@S@_over(
    struct dj_internal_math_@S@_pair _u,
    struct dj_internal_math_@S@_pair _n
)
{
    const @T@ q0    = _u.high / _n.high;
    const @T@ least =
        @MIN@ * d_internal_math_@S@_pow2(@P@ + 4);

    // a zero: itself, its sign kept
    if (q0 == (@T@)0)
    {
        return q0;
    }

    // too near the subnormals for Dekker's product
    if (d_internal_cplx_@S@_abs(q0) < least)
    {
        return q0 - (q0 * (_n.low / _n.high));
    }

    {
        const struct dj_internal_math_@S@_pair t =
            d_internal_cplx_@S@_product(q0, _n.high);
        // u - q n: u's high part less t's is exact, the two within an ulp
        const @T@ head = (_u.high - t.high) - t.low;
        const @T@ r    = (head + _u.low) - (q0 * _n.low);

        return q0 + (r / _n.high);
    }
}

// d_internal_cplx_@S@_exponent
//   the binade E of a finite nonzero _x, 2^E <= |_x| < 2^(E + 1): a
// subnormal scaled into the normal range first.
D_MATH_FN int
d_internal_cplx_@S@_exponent(
    @T@ _x
)
{
    const @T@ a    = d_internal_cplx_@S@_abs(_x);
    const int lift = (a < @MIN@) ? @P@ : 0;
    const @T@ v    = (lift > 0) ? (a * d_internal_math_@S@_pow2(lift)) : a;

    return d_internal_math_@S@_binade_of(v).exponent - lift;
}

// d_internal_cplx_@S@_scale
//   _x 2^_k, rounded once: in steps of at most 2^(EMAX - 1), so that no step
// overflows or underflows before the last, which rounds where the result is
// subnormal.
D_MATH_FN @T@
d_internal_cplx_@S@_scale(
    @T@ _x,
    int _k
)
{
    @T@ value = _x;
    int left  = _k;

    // whole steps up or down while more than one is left
    while (left > (@EMAX@ - 1))
    {
        value = value * d_internal_math_@S@_pow2(@EMAX@ - 1);
        left  = left - (@EMAX@ - 1);
    }

    while (left < (@EMIN@ - 1))
    {
        value = value * d_internal_math_@S@_pow2(@EMIN@ - 1);
        left  = left - (@EMIN@ - 1);
    }

    return value * d_internal_math_@S@_pow2(left);
}

// d_internal_cplx_@S@_magnitude_exponent
//   the binade of the larger part's magnitude of _z, both parts finite and
// one nonzero.
D_MATH_FN int
d_internal_cplx_@S@_magnitude_exponent(
    struct dj_cplx_@S@ _z
)
{
    const @T@ a = d_internal_cplx_@S@_abs(_z.part[0]);
    const @T@ b = d_internal_cplx_@S@_abs(_z.part[1]);

    return d_internal_cplx_@S@_exponent((a > b) ? a : b);
}

// d_internal_cplx_@S@_safe
//   whether every part of _z lies where Dekker's products of the parts with
// another value's of the same reach neither overflow nor lose their low
// parts: from 2^(EMIN/2 + P) to 2^(EMAX/2 - P), or 0.
D_MATH_FN bool
d_internal_cplx_@S@_safe(
    struct dj_cplx_@S@ _z
)
{
    const @T@ low  = d_internal_math_@S@_pow2((@EMIN@ / 2) + @P@);
    const @T@ high = d_internal_math_@S@_pow2((@EMAX@ / 2) - @P@);
    const @T@ a    = d_internal_cplx_@S@_abs(_z.part[0]);
    const @T@ b    = d_internal_cplx_@S@_abs(_z.part[1]);

    return ( ( (a == (@T@)0) ||
               ( (a >= low) &&
                 (a <= high) ) ) &&
             ( (b == (@T@)0) ||
               ( (b >= low) &&
                 (b <= high) ) ) );
}

// d_internal_cplx_@S@_finite
//   whether both parts of _z are finite.
D_MATH_FN bool
d_internal_cplx_@S@_finite(
    struct dj_cplx_@S@ _z
)
{
    return ( (_z.part[0] == _z.part[0]) &&
             (_z.part[1] == _z.part[1]) &&
             (!d_internal_cplx_@S@_is_inf(_z.part[0])) &&
             (!d_internal_cplx_@S@_is_inf(_z.part[1])) );
}

// d_internal_cplx_@S@_scaled_by
//   both parts of _z times 2^_k, each rounded once.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_scaled_by(
    struct dj_cplx_@S@ _z,
    int                _k
)
{
    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_scale(_z.part[0], _k),
        d_internal_cplx_@S@_scale(_z.part[1], _k));
}

// d_internal_cplx_@S@_mul_finite
//   _z _w, both finite: each operand scaled to a larger part in [1, 2) where
// either leaves the safe range, the parts by d_internal_cplx_@S@_dot, the
// result scaled back.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_mul_finite(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    // within the safe range, or a zero operand: the parts as they are
    if ( ( (d_internal_cplx_@S@_safe(_z)) &&
           (d_internal_cplx_@S@_safe(_w)) ) ||
         ( (_z.part[0] == (@T@)0) &&
           (_z.part[1] == (@T@)0) ) ||
         ( (_w.part[0] == (@T@)0) &&
           (_w.part[1] == (@T@)0) ) )
    {
        return d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_dot(_z.part[0], _w.part[0],
                                    -_z.part[1], _w.part[1]),
            d_internal_cplx_@S@_dot(_z.part[0], _w.part[1],
                                    _z.part[1], _w.part[0]));
    }

    {
        // each operand's larger part into [1, 2); what lies far below it
        // may underflow in the products, under 2^-2P of the result
        const int ez = d_internal_cplx_@S@_magnitude_exponent(_z);
        const int ew = d_internal_cplx_@S@_magnitude_exponent(_w);
        const struct dj_cplx_@S@ z = d_internal_cplx_@S@_scaled_by(_z, -ez);
        const struct dj_cplx_@S@ w = d_internal_cplx_@S@_scaled_by(_w, -ew);
        const struct dj_cplx_@S@ p = d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_dot(z.part[0], w.part[0],
                                    -z.part[1], w.part[1]),
            d_internal_cplx_@S@_dot(z.part[0], w.part[1],
                                    z.part[1], w.part[0]));

        return d_internal_cplx_@S@_scaled_by(p, ez + ew);
    }
}

// d_internal_cplx_@S@_unit_or_zero
//   +-1 for an infinity and +-0 for any other value, the sign _x's: Annex G's
// boxing of an infinite operand (G.5.1, example 1).
D_MATH_FN @T@
d_internal_cplx_@S@_unit_or_zero(
    @T@ _x
)
{
    return d_internal_cplx_@S@_copysign(
        d_internal_cplx_@S@_is_inf(_x) ? (@T@)1 : (@T@)0,
        _x);
}

// d_internal_cplx_@S@_zero_if_nan
//   _x, or +-0 for a NaN, its sign bit kept.
D_MATH_FN @T@
d_internal_cplx_@S@_zero_if_nan(
    @T@ _x
)
{
    return (_x != _x) ? d_internal_cplx_@S@_copysign((@T@)0, _x) : _x;
}

// d_cplx_@S@_make
//   function: _x + _y i, each part as given, a signed zero, an infinity or a
// NaN among them - what C11's CMPLX makes and the sum _x + _y i, for a NaN
// or an infinite _y, cannot.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_make(
    @T@ _x,
    @T@ _y
)
{
    return d_internal_cplx_@S@_make(_x,
                                    _y);
}

// d_cplx_@S@_real, d_cplx_@S@_imag
//   function: the real and the imaginary part of _z (C99's creal, cimag).
D_MATH_FN @T@
d_cplx_@S@_real(
    struct dj_cplx_@S@ _z
)
{
    return _z.part[0];
}

D_MATH_FN @T@
d_cplx_@S@_imag(
    struct dj_cplx_@S@ _z
)
{
    return _z.part[1];
}

// d_cplx_@S@_conj, d_cplx_@S@_proj, d_cplx_@S@_neg
//   function: the conjugate, the imaginary part negated (C99's conj); the
// projection onto the Riemann sphere, every infinity to +inf + i(+-0), the
// zero's sign the imaginary part's (C99's cproj); and the negation.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_conj(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_z.part[0],
                                    -_z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_proj(
    struct dj_cplx_@S@ _z
)
{
    // an infinite part, the other perhaps a NaN: the one infinity
    if ( (d_internal_cplx_@S@_is_inf(_z.part[0])) ||
         (d_internal_cplx_@S@_is_inf(_z.part[1])) )
    {
        return d_internal_cplx_@S@_make(
            d_internal_math_@S@_infinity(),
            d_internal_cplx_@S@_copysign((@T@)0, _z.part[1]));
    }

    return _z;
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_neg(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(-_z.part[0],
                                    -_z.part[1]);
}

// d_cplx_@S@_add, d_cplx_@S@_sub
//   function: _z + _w and _z - _w, part by part.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_add(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_make(_z.part[0] + _w.part[0],
                                    _z.part[1] + _w.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_sub(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_make(_z.part[0] - _w.part[0],
                                    _z.part[1] - _w.part[1]);
}

// d_cplx_@S@_mul
//   function: _z _w, each part within 2 ulps where no part of the operands
// is an infinity or a NaN; and where the formula's terms give NaN for both
// parts while an operand is infinite, the infinity Annex G recovers (G.5.1).
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_mul(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    // finite operands: the exact products
    if ( (d_internal_cplx_@S@_finite(_z)) &&
         (d_internal_cplx_@S@_finite(_w)) )
    {
        return d_internal_cplx_@S@_mul_finite(_z, _w);
    }

    {
        const @T@ a = _z.part[0];
        const @T@ b = _z.part[1];
        const @T@ c = _w.part[0];
        const @T@ d = _w.part[1];
        const @T@ x = (a * c) - (b * d);
        const @T@ y = (a * d) + (b * c);

        // a part not NaN: the formula's
        if ( (x == x) ||
             (y == y) )
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        // an infinite operand: its parts boxed, the other's NaNs as 0s
        if ( (d_internal_cplx_@S@_is_inf(a)) ||
             (d_internal_cplx_@S@_is_inf(b)) ||
             (d_internal_cplx_@S@_is_inf(c)) ||
             (d_internal_cplx_@S@_is_inf(d)) )
        {
            const bool z_inf = ( (d_internal_cplx_@S@_is_inf(a)) ||
                                 (d_internal_cplx_@S@_is_inf(b)) );
            const bool w_inf = ( (d_internal_cplx_@S@_is_inf(c)) ||
                                 (d_internal_cplx_@S@_is_inf(d)) );
            const @T@ a1 = z_inf ? d_internal_cplx_@S@_unit_or_zero(a)
                                 : d_internal_cplx_@S@_zero_if_nan(a);
            const @T@ b1 = z_inf ? d_internal_cplx_@S@_unit_or_zero(b)
                                 : d_internal_cplx_@S@_zero_if_nan(b);
            const @T@ c1 = w_inf ? d_internal_cplx_@S@_unit_or_zero(c)
                                 : d_internal_cplx_@S@_zero_if_nan(c);
            const @T@ d1 = w_inf ? d_internal_cplx_@S@_unit_or_zero(d)
                                 : d_internal_cplx_@S@_zero_if_nan(d);
            const @T@ inf = d_internal_math_@S@_infinity();

            return d_internal_cplx_@S@_make(
                inf * ((a1 * c1) - (b1 * d1)),
                inf * ((a1 * d1) + (b1 * c1)));
        }

        return d_internal_cplx_@S@_make(x, y);
    }
}

// d_internal_cplx_@S@_div_finite
//   _z / _w, both finite and _w not 0: both scaled to a larger part in
// [1, 2), so that the squared modulus n of _w lies in [1, 8) and every
// product below is exact; the numerator's parts as pairs, each over n by
// d_internal_cplx_@S@_over, the result scaled back by the operands'
// exponents. Where a target evaluates float and double wider than their
// types, n and the numerator are long double pairs, as in
// d_internal_cplx_@S@_dot_wide, and each quotient the _over_wide one's.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_div_finite(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const int ew = d_internal_cplx_@S@_magnitude_exponent(_w);
    const struct dj_cplx_@S@ w = d_internal_cplx_@S@_scaled_by(_w, -ew);
    const bool zero = ( (_z.part[0] == (@T@)0) &&
                        (_z.part[1] == (@T@)0) );
    const int ez = zero ? 0 : d_internal_cplx_@S@_magnitude_exponent(_z);
    const struct dj_cplx_@S@ z = d_internal_cplx_@S@_scaled_by(_z, -ez);
#if @WIDE@
    // n = c^2 + d^2, exactly as a long double pair, then one pair again
    const struct dj_internal_math_ld_pair cc =
        d_internal_math_ld_product_exact((long double)w.part[0],
                                         (long double)w.part[0]);
    const struct dj_internal_math_ld_pair dd =
        d_internal_math_ld_product_exact((long double)w.part[1],
                                         (long double)w.part[1]);
    const struct dj_internal_math_ld_pair n0 =
        d_internal_math_ld_two_sum(cc.high, dd.high);
    const struct dj_internal_math_ld_pair n =
        d_internal_math_ld_two_sum(n0.high, n0.low + (cc.low + dd.low));
    // the numerator (a + bi)(c - di), each part a long double pair
    const struct dj_internal_math_ld_pair re =
        d_internal_cplx_@S@_dot_wide(z.part[0], w.part[0],
                                     z.part[1], w.part[1]);
    const struct dj_internal_math_ld_pair im =
        d_internal_cplx_@S@_dot_wide(z.part[1], w.part[0],
                                     -z.part[0], w.part[1]);

    return d_internal_cplx_@S@_scaled_by(
        d_internal_cplx_@S@_make(d_internal_cplx_@S@_over_wide(re, n),
                                 d_internal_cplx_@S@_over_wide(im, n)),
        ez - ew);
#else  // @WIDE@
    // n = c^2 + d^2, exactly as high + low, then one pair again
    const struct dj_internal_math_@S@_pair cc =
        d_internal_cplx_@S@_product(w.part[0], w.part[0]);
    const struct dj_internal_math_@S@_pair dd =
        d_internal_cplx_@S@_product(w.part[1], w.part[1]);
    const struct dj_internal_math_@S@_pair n0 =
        d_internal_cplx_@S@_two_sum(cc.high, dd.high);
    const struct dj_internal_math_@S@_pair n =
        d_internal_cplx_@S@_two_sum(n0.high, n0.low + (cc.low + dd.low));
    // the numerator (a + bi)(c - di), each part a pair
    const struct dj_internal_math_@S@_pair re =
        d_internal_cplx_@S@_dot_pair(z.part[0], w.part[0],
                                     z.part[1], w.part[1]);
    const struct dj_internal_math_@S@_pair im =
        d_internal_cplx_@S@_dot_pair(z.part[1], w.part[0],
                                     -z.part[0], w.part[1]);

    return d_internal_cplx_@S@_scaled_by(
        d_internal_cplx_@S@_make(d_internal_cplx_@S@_over(re, n),
                                 d_internal_cplx_@S@_over(im, n)),
        ez - ew);
#endif  // @WIDE@
}

// d_cplx_@S@_div
//   function: _z / _w, each part within a step of the correctly rounded one
// (d_internal_cplx_@S@_over) where no part of the operands
// is an infinity or a NaN and _w is not 0; and where the formula gives NaN
// for both parts, the infinity or the zero Annex G recovers (G.5.1): an
// infinity for a nonzero _z over 0, an infinity for an infinite _z over a
// finite _w, a zero for a finite _z over an infinite _w.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_div(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const bool w_zero = ( (_w.part[0] == (@T@)0) &&
                          (_w.part[1] == (@T@)0) );

    // finite operands, _w not 0: the scaled quotient
    if ( (d_internal_cplx_@S@_finite(_z)) &&
         (d_internal_cplx_@S@_finite(_w)) &&
         (!w_zero) )
    {
        return d_internal_cplx_@S@_div_finite(_z, _w);
    }

    {
        const @T@ a   = _z.part[0];
        const @T@ b   = _z.part[1];
        const @T@ c   = _w.part[0];
        const @T@ d   = _w.part[1];
        const @T@ inf = d_internal_math_@S@_infinity();

        // over 0: an infinity in each part, the signs the formula's, where
        // _z is not NaN through
        if ( (w_zero) &&
             ( (a == a) ||
               (b == b) ) )
        {
            const @T@ s = d_internal_cplx_@S@_copysign(inf, c);

            return d_internal_cplx_@S@_make(s * a, s * b);
        }

        // an infinite _z over a finite _w
        if ( ( (d_internal_cplx_@S@_is_inf(a)) ||
               (d_internal_cplx_@S@_is_inf(b)) ) &&
             (d_internal_cplx_@S@_finite(_w)) )
        {
            const @T@ a1 = d_internal_cplx_@S@_unit_or_zero(a);
            const @T@ b1 = d_internal_cplx_@S@_unit_or_zero(b);

            return d_internal_cplx_@S@_make(inf * ((a1 * c) + (b1 * d)),
                                            inf * ((b1 * c) - (a1 * d)));
        }

        // a finite _z over an infinite _w
        if ( ( (d_internal_cplx_@S@_is_inf(c)) ||
               (d_internal_cplx_@S@_is_inf(d)) ) &&
             (d_internal_cplx_@S@_finite(_z)) )
        {
            const @T@ c1 = d_internal_cplx_@S@_unit_or_zero(c);
            const @T@ d1 = d_internal_cplx_@S@_unit_or_zero(d);

            return d_internal_cplx_@S@_make((@T@)0 * ((a * c1) + (b * d1)),
                                            (@T@)0 * ((b * c1) - (a * d1)));
        }

        // every other: NaNs
        {
            const @T@ nan = d_internal_math_@S@_nan();

            return d_internal_cplx_@S@_make(nan, nan);
        }
    }
}

// d_cplx_@S@_add_real, d_cplx_@S@_sub_real, d_cplx_@S@_real_sub,
// d_cplx_@S@_mul_real, d_cplx_@S@_div_real, d_cplx_@S@_real_div
//   function: an operation of a complex _z and a real _x, the real taken as
// itself, not as _x + 0i, as Annex G takes a real operand (G.5): _z + _x,
// _z - _x and _x - _z keep _z's imaginary part, its zero's sign among it;
// _z _x and _z / _x scale both parts, so that an infinite _z by a finite _x
// is no NaN; _x / _z is _x over _z, as d_cplx_@S@_div gives it.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_add_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] + _x,
                                    _z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_sub_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] - _x,
                                    _z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_real_sub(
    @T@                _x,
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_x - _z.part[0],
                                    -_z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_mul_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] * _x,
                                    _z.part[1] * _x);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_div_real(
    struct dj_cplx_@S@ _z,
    @T@                _x
)
{
    return d_internal_cplx_@S@_make(_z.part[0] / _x,
                                    _z.part[1] / _x);
}

D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_real_div(
    @T@                _x,
    struct dj_cplx_@S@ _z
)
{
    return d_cplx_@S@_div(d_internal_cplx_@S@_make(_x, (@T@)0),
                          _z);
}

// d_cplx_@S@_equal
//   function: whether _z and _w are equal part by part, as == finds them: a
// NaN part equals nothing, and -0 equals +0.
D_MATH_FN bool
d_cplx_@S@_equal(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return ( (_z.part[0] == _w.part[0]) &&
             (_z.part[1] == _w.part[1]) );
}
"""

# the kernels of C99's functions, in @T@ on the real functions of suffix @R@,
# their names suffixed @KS@ and qualified @KQ@: once in long double on the
# _c real functions (@KS@ = @R@ = _c), and once in each family's own type on
# the C library's (@KS@ = @R@ = empty)
CPLX_KERNELS = r"""
// @SECTION@.@SUB@    @TITLE@
//------------------------------------------------------------------------------

// d_internal_cplx_@S@_exp_times@KS@
//   e^_x _c, _x finite, without the overflow of e^_x where the product is in
// range: past ln(MAX) as (e^(x/2) _c) e^(x/2), x/2 exact, within 3 ulps.
@KQ@ @T@
d_internal_cplx_@S@_exp_times@KS@(
    @T@ _x,
    @T@ _c
)
{
    // within range: the exponential itself
    if (_x <= ((@T@)(@EMAX@ - 1) * (@T@)0.6931L))
    {
        return d_math_@S@_exp@R@(_x) * _c;
    }

    {
        const @T@ h = d_math_@S@_exp@R@(_x / (@T@)2);

        return (h * _c) * h;
    }
}

// d_internal_cplx_@S@_exp@KS@
//   e^_z: e^x (cos y + i sin y), and Annex G's values (G.6.3.1) where a part
// is an infinity or a NaN: conj(exp z) = exp(conj z) throughout.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_exp@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // a zero imaginary part: e^x and that zero, NaN + i0 among them
    if (y == (@T@)0)
    {
        return d_internal_cplx_@S@_make(d_math_@S@_exp@R@(x), y);
    }

    // x a NaN, y not 0: NaNs
    if (x != x)
    {
        return d_internal_cplx_@S@_make(x, x);
    }

    // y infinite or NaN
    if ( (y != y) ||
         (d_internal_cplx_@S@_is_inf(y)) )
    {
        // -inf: +-0 +- i0; +inf: +-inf + iNaN; finite x: NaNs
        if (d_internal_cplx_@S@_is_inf(x))
        {
            if (x < (@T@)0)
            {
                return d_internal_cplx_@S@_make((@T@)0, (@T@)0);
            }

            return d_internal_cplx_@S@_make(x, nan);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // x infinite, y finite and not 0: +0 cis(y) or +inf cis(y)
    if (d_internal_cplx_@S@_is_inf(x))
    {
        const @T@ m = (x < (@T@)0) ? (@T@)0 : x;

        return d_internal_cplx_@S@_make(m * d_math_@S@_cos@R@(y),
                                        m * d_math_@S@_sin@R@(y));
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_exp_times@KS@(x, d_math_@S@_cos@R@(y)),
        d_internal_cplx_@S@_exp_times@KS@(x, d_math_@S@_sin@R@(y)));
}

// d_internal_cplx_@S@_log_modulus@KS@
//   log |_z|, both parts finite and one nonzero: near the unit circle, for
// |z|^2 in [1/2, 2], log1p(x^2 + y^2 - 1) / 2, the argument's terms exact
// and summed so that only small ones round; elsewhere log(hypot(x, y)),
// scaled by a power of 2 past half the range or into the normal numbers, the
// scale's ln 2 added back.
@KQ@ @T@
d_internal_cplx_@S@_log_modulus@KS@(
    @T@ _x,
    @T@ _y
)
{
    const @T@ a = d_internal_cplx_@S@_abs(_x);
    const @T@ b = d_internal_cplx_@S@_abs(_y);
    const @T@ big   = (a > b) ? a : b;
    const @T@ small = (a > b) ? b : a;

    // near the unit circle: |z|^2 - 1 exactly as far as it goes
    if ( (big >= (@T@)0.5) &&
         (big <= (@T@)1.5) &&
         (small < (@T@)1.0) )
    {
#if @WIDE@
        // the squares and their sums as long double pairs, as the
        // arithmetic's are where @T@ is evaluated wider (section 2)
        const struct dj_internal_math_ld_pair bb =
            d_internal_math_ld_product_exact((long double)big,
                                             (long double)big);
        const struct dj_internal_math_ld_pair ss =
            d_internal_math_ld_product_exact((long double)small,
                                             (long double)small);
        const long double m2 = bb.high + (ss.high + ss.low);

        if ( (m2 >= 0.5L) &&
             (m2 <= 2.0L) )
        {
            const struct dj_internal_math_ld_pair t1 =
                d_internal_math_ld_two_sum(bb.high, -1.0L);
            const struct dj_internal_math_ld_pair head =
                d_internal_math_ld_two_sum(t1.high, ss.high);
            const struct dj_internal_math_ld_pair v =
                d_internal_math_ld_two_sum(
                    head.high,
                    ((head.low + t1.low) + bb.low) + ss.low);

            return d_math_@S@_log1p@R@(
                       d_internal_math_@S@_round_pair(v.high, v.low)) /
                   (@T@)2;
        }
#else  // @WIDE@
        const struct dj_internal_math_@S@_pair bb =
            d_internal_cplx_@S@_product(big, big);
        const struct dj_internal_math_@S@_pair ss =
            d_internal_cplx_@S@_product(small, small);
        const @T@ s2 = ss.high + ss.low;
        const @T@ m2 = bb.high + s2;

        if ( (m2 >= (@T@)0.5) &&
             (m2 <= (@T@)2) )
        {
            // b^2 - 1 exactly as a pair: Sterbenz's lemma makes it one value
            // only where b^2 is 1/2 or more, and both parts near 1/sqrt(2)
            // put it below; then the small squares
            const struct dj_internal_math_@S@_pair t1 =
                d_internal_cplx_@S@_two_sum(bb.high, -(@T@)1);
            const struct dj_internal_math_@S@_pair head =
                d_internal_cplx_@S@_two_sum(t1.high, ss.high);
            const @T@ v = head.high +
                          (((head.low + t1.low) + bb.low) + ss.low);

            return d_math_@S@_log1p@R@(v) / (@T@)2;
        }
#endif  // @WIDE@
    }

    // past half the range: both halved, ln 2 added
    if (big > (@MAX@ / (@T@)2))
    {
        return d_math_@S@_log@R@(d_math_@S@_hypot@R@(_x / (@T@)2,
                                                     _y / (@T@)2)) +
               (@T@)0.693147180559945309417232121458176568L;
    }

    // below the normal numbers: both raised by 2^P, P ln 2 taken off
    if (big < (@MIN@ * d_internal_math_@S@_pow2(@P@)))
    {
        const @T@ lift = d_internal_math_@S@_pow2(@P@);

        return d_math_@S@_log@R@(d_math_@S@_hypot@R@(_x * lift, _y * lift)) -
               ((@T@)@P@ * (@T@)0.693147180559945309417232121458176568L);
    }

    return d_math_@S@_log@R@(d_math_@S@_hypot@R@(_x, _y));
}

// d_internal_cplx_@S@_log@KS@
//   log _z: log |z| + i arg z, the angle atan2's, so that Annex G's values
// (G.6.3.2) follow from hypot's and atan2's own - log(+-0 + i0) is -inf + i0
// or i pi, an infinite part gives +inf, a NaN with a finite part NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x = _z.part[0];
    const @T@ y = _z.part[1];
    const @T@ angle = d_math_@S@_atan2@R@(y, x);

    // zero, an infinite part or a NaN: hypot's logarithm
    if ( ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         (!d_internal_cplx_@S@_finite(_z)) )
    {
        return d_internal_cplx_@S@_make(
            d_math_@S@_log@R@(d_math_@S@_hypot@R@(x, y)),
            angle);
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_log_modulus@KS@(x, y),
        angle);
}

// d_internal_cplx_@S@_sqrt@KS@
//   sqrt _z, the principal root, Kahan's: t = sqrt((|x| + |z|) / 2), then
// (t, y / 2t) for x >= 0 and (|y| / 2t, +-t) for x < 0, the sign y's;
// scaled by 4^k where |x| + |z| would overflow or underflow. Annex G's values
// (G.6.4.2): an infinite y gives +inf + i y, a NaN with a finite part NaNs,
// -inf gives +0 + i(+-inf) and +inf gives +inf + i(+-0).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_sqrt@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ inf = d_internal_math_@S@_infinity();

    // an infinite imaginary part, x anything: +inf + i y
    if (d_internal_cplx_@S@_is_inf(y))
    {
        return d_internal_cplx_@S@_make(inf, y);
    }

    // x infinite
    if (d_internal_cplx_@S@_is_inf(x))
    {
        // -inf: NaN +- i inf for a NaN y, else +0 + i(+-inf)
        if (x < (@T@)0)
        {
            if (y != y)
            {
                return d_internal_cplx_@S@_make(y, inf);
            }

            return d_internal_cplx_@S@_make(
                (@T@)0,
                d_internal_cplx_@S@_copysign(inf, y));
        }

        // +inf: +inf + iNaN, or +inf + i(+-0)
        if (y != y)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        return d_internal_cplx_@S@_make(
            x,
            d_internal_cplx_@S@_copysign((@T@)0, y));
    }

    // a NaN with the rest finite: NaNs
    if ( (x != x) ||
         (y != y) )
    {
        const @T@ nan = d_internal_math_@S@_nan();

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // zero: +0 and y's zero
    if ( (x == (@T@)0) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make((@T@)0, y);
    }

    {
        const @T@ a   = d_internal_cplx_@S@_abs(x);
        const @T@ b   = d_internal_cplx_@S@_abs(y);
        const @T@ big = (a > b) ? a : b;
        // past a quarter of the range, the parts over 4 and t times 2;
        // below the normal numbers, times 4^k and t over 2^k
        const int down = (big > (@MAX@ / (@T@)4)) ? 1 : 0;
        const int up   = (big < (@MIN@ * d_internal_math_@S@_pow2(@P@)))
                             ? ((@P@ + 1) / 2)
                             : 0;
        const int k    = up - down;
        const @T@ sa   = d_internal_cplx_@S@_scale(a, 2 * k);
        const @T@ sb   = d_internal_cplx_@S@_scale(b, 2 * k);
        const @T@ m    = d_math_@S@_hypot@R@(sa, sb);
        const @T@ st   = d_math_@S@_sqrt@R@((sa + m) / (@T@)2);
        // t and |y| / 2t in the scaled numbers, then back
        const @T@ other = (sb / st) / (@T@)2;
        const @T@ t     = d_internal_cplx_@S@_scale(st, -k);
        const @T@ u     = d_internal_cplx_@S@_scale(other, -k);

        if (x >= (@T@)0)
        {
            return d_internal_cplx_@S@_make(
                t,
                d_internal_cplx_@S@_copysign(u, y));
        }

        return d_internal_cplx_@S@_make(
            u,
            d_internal_cplx_@S@_copysign(t, y));
    }
}

// d_internal_cplx_@S@_cispi@KS@
//   cos(pi _r) + i sin(pi _r), |_r| <= 1, by the octant of _r: its distance
// from 0, 1/2 or 1, exact by Sterbenz's lemma, times pi, so that the angle
// handed to sin and cos is small and accurate, and 0, 1/2 and 1 give their
// exact values.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cispi@KS@(
    @T@ _r
)
{
    const @T@ pi = (@T@)3.141592653589793238462643383279502884L;
    const bool negative = d_internal_math_@S@_negative(_r);
    const @T@ r = negative ? -_r : _r;
    @T@ c = (@T@)1;
    @T@ s = (@T@)0;

    // [0, 1/4]: cos(pi r), sin(pi r)
    if (r <= (@T@)0.25)
    {
        c = d_math_@S@_cos@R@(pi * r);
        s = d_math_@S@_sin@R@(pi * r);
    }
    // (1/4, 3/4]: sin(pi (1/2 - r)), cos(pi (1/2 - r))
    else if (r <= (@T@)0.75)
    {
        const @T@ t = (@T@)0.5 - r;

        c = d_math_@S@_sin@R@(pi * t);
        s = d_math_@S@_cos@R@(pi * t);
    }
    // (3/4, 1]: -cos(pi (1 - r)), sin(pi (1 - r))
    else
    {
        const @T@ t = (@T@)1 - r;

        c = -d_math_@S@_cos@R@(pi * t);
        s = d_math_@S@_sin@R@(pi * t);
    }

    return d_internal_cplx_@S@_make(c, negative ? -s : s);
}

// d_internal_cplx_@S@_turn@KS@
//   cos(pi _a) + i sin(pi _a) for any finite _a: _a less its nearest even
// integer, exact, into [-1, 1], then cispi.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_turn@KS@(
    @T@ _a
)
{
    const @T@ half = _a / (@T@)2;
    const @T@ k    = d_math_@S@_floor_c(half + (@T@)0.5);
    const @T@ r    = _a - ((@T@)2 * k);

    return d_internal_cplx_@S@_cispi@KS@(r);
}

@POW_GENERAL@
@POW_INT@// d_internal_cplx_@S@_pow@KS@
//   _z^_w, the principal value exp(w log z), with these exact: z^0 is 1 for
// every z; a whole w, from -2^30 to 2^30, repeated squaring; a positive real
// z to a real w the real pow, and a negative real or imaginary one |z|^w
// times cos + i sin of w pi or w pi/2, by turns, so that (-4)^(1/2) is 2i;
// 0^w is 0 for Re w > 0, +inf for a real w < 0, and NaNs otherwise.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow@KS@(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ a   = _w.part[0];
    const @T@ b   = _w.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // z^0 is 1
    if ( (a == (@T@)0) &&
         (b == (@T@)0) )
    {
        return d_internal_cplx_@S@_make((@T@)1, (@T@)0);
    }

    // a NaN anywhere: NaNs
    if ( (x != x) ||
         (y != y) ||
         (a != a) ||
         (b != b) )
    {
        return d_internal_cplx_@S@_make(nan, nan);
    }

    // a real exponent
    if (b == (@T@)0)
    {
        // whole, of an int: repeated squaring of a finite z not 0
        if ( (d_math_@S@_floor_c(a) == a) &&
             (a >= (@T@)-1073741824.0L) &&
             (a <= (@T@)1073741824.0L) &&
             (d_internal_cplx_@S@_finite(_z)) &&
             ( (x != (@T@)0) ||
               (y != (@T@)0) ) )
        {
            return d_internal_cplx_@S@_pow_int@KS@(_z, (int)a);
        }

        // a real z
        if (y == (@T@)0)
        {
            // a positive one: the real pow, the zero's sign by w's
            if (x > (@T@)0)
            {
                return d_internal_cplx_@S@_make(d_math_@S@_pow@R@(x, a),
                                                a * y);
            }

            // a negative one: |x|^w (cos + i sin)(w pi), the sign of pi y's
            if (x < (@T@)0)
            {
                const @T@ r = d_math_@S@_pow@R@(-x, a);
                const struct dj_cplx_@S@ c = d_internal_cplx_@S@_turn@KS@(
                    d_internal_math_@S@_negative(y) ? -a : a);

                return d_cplx_@S@_mul_real(c, r);
            }
        }

        // an imaginary z: |y|^w (cos + i sin)(+-w pi/2)
        if ( (x == (@T@)0) &&
             (y != (@T@)0) &&
             (!d_internal_cplx_@S@_is_inf(y)) )
        {
            const @T@ r = d_math_@S@_pow@R@(d_internal_cplx_@S@_abs(y), a);
            const struct dj_cplx_@S@ c = d_internal_cplx_@S@_turn@KS@(
                (y < (@T@)0) ? (-a / (@T@)2) : (a / (@T@)2));

            return d_cplx_@S@_mul_real(c, r);
        }
    }

    // 0^w
    if ( (x == (@T@)0) &&
         (y == (@T@)0) )
    {
        if (a > (@T@)0)
        {
            return d_internal_cplx_@S@_make((@T@)0, (@T@)0);
        }

        if ( (a < (@T@)0) &&
             (b == (@T@)0) )
        {
            return d_internal_cplx_@S@_make(d_internal_math_@S@_infinity(),
                                            (@T@)0);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    return d_internal_cplx_@S@_pow_general@KS@(_z, _w);
}

// d_internal_cplx_@S@_cosh_sinh_times@KS@
//   cosh(_x) _c and sinh(_x) _c at once, _x finite: past ln(MAX) both are
// e^|x| / 2 times _c, by the exponential's scaled product, the sine's sign
// x's.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cosh_sinh_times@KS@(
    @T@ _x,
    @T@ _c,
    @T@ _s
)
{
    const @T@ ax = d_internal_cplx_@S@_abs(_x);

    // within range: cosh x _c and sinh x _s themselves
    if (ax <= ((@T@)(@EMAX@ - 1) * (@T@)0.6931L))
    {
        return d_internal_cplx_@S@_make(d_math_@S@_cosh@R@(_x) * _c,
                                        d_math_@S@_sinh@R@(_x) * _s);
    }

    {
        const @T@ half_c = d_internal_cplx_@S@_exp_times@KS@(ax,
                                                             _c / (@T@)2);
        const @T@ half_s = d_internal_cplx_@S@_exp_times@KS@(ax,
                                                             _s / (@T@)2);

        return d_internal_cplx_@S@_make(half_c,
                                        (_x < (@T@)0) ? -half_s : half_s);
    }
}

// d_internal_cplx_@S@_sinh@KS@
//   sinh _z = sinh x cos y + i cosh x sin y, Annex G's values (G.6.2.5) at
// the infinities and NaNs: it is odd and conj(sinh z) = sinh(conj z).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_sinh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // a finite argument
    if (d_internal_cplx_@S@_finite(_z))
    {
        // y = 0: sinh x and y's zero
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(d_math_@S@_sinh@R@(x), y);
        }

        {
            const struct dj_cplx_@S@ ch =
                d_internal_cplx_@S@_cosh_sinh_times@KS@(
                    x,
                    d_math_@S@_sin@R@(y),
                    d_math_@S@_cos@R@(y));

            return d_internal_cplx_@S@_make(ch.part[1], ch.part[0]);
        }
    }

    // x 0, y infinite or NaN: +-0 + iNaN
    if (x == (@T@)0)
    {
        return d_internal_cplx_@S@_make(x, nan);
    }

    // x infinite
    if (d_internal_cplx_@S@_is_inf(x))
    {
        // y 0: x + i y
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        // y infinite or NaN: +-inf + iNaN
        if ( (y != y) ||
             (d_internal_cplx_@S@_is_inf(y)) )
        {
            return d_internal_cplx_@S@_make(x, nan);
        }

        // y finite: inf cis(y), the real part's sign x's
        return d_internal_cplx_@S@_make(x * d_math_@S@_cos@R@(y),
                                        d_internal_cplx_@S@_copysign(
                                            x, (@T@)1) *
                                        d_math_@S@_sin@R@(y));
    }

    // x NaN: NaN + i0 for y = 0, else NaNs
    if ( (x != x) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make(x, y);
    }

    return d_internal_cplx_@S@_make(nan, nan);
}

// d_internal_cplx_@S@_cosh@KS@
//   cosh _z = cosh x cos y + i sinh x sin y, Annex G's values (G.6.2.4) at
// the infinities and NaNs: it is even and conj(cosh z) = cosh(conj z).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cosh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // a finite argument
    if (d_internal_cplx_@S@_finite(_z))
    {
        // y = 0: cosh x and the zero x y gives
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(d_math_@S@_cosh@R@(x), x * y);
        }

        return d_internal_cplx_@S@_cosh_sinh_times@KS@(
            x,
            d_math_@S@_cos@R@(y),
            d_math_@S@_sin@R@(y));
    }

    // x 0, y infinite or NaN: NaN + i0
    if (x == (@T@)0)
    {
        return d_internal_cplx_@S@_make(nan, (@T@)0);
    }

    // x infinite
    if (d_internal_cplx_@S@_is_inf(x))
    {
        const @T@ inf = d_internal_math_@S@_infinity();

        // y 0: +inf + i0, the zero signed as sinh x sin y is
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(
                inf,
                d_internal_cplx_@S@_copysign((@T@)0, x) * y);
        }

        // y infinite or NaN: +inf + iNaN
        if ( (y != y) ||
             (d_internal_cplx_@S@_is_inf(y)) )
        {
            return d_internal_cplx_@S@_make(inf, nan);
        }

        // y finite: +inf cis(y), the sine's term signed by x
        return d_internal_cplx_@S@_make(inf * d_math_@S@_cos@R@(y),
                                        x * d_math_@S@_sin@R@(y));
    }

    // x NaN: NaN + i0 for y = 0, else NaNs
    if (y == (@T@)0)
    {
        return d_internal_cplx_@S@_make(x, (@T@)0);
    }

    return d_internal_cplx_@S@_make(nan, nan);
}

// d_internal_cplx_@S@_tanh@KS@
//   tanh _z by Kahan's formula: t = tan y, b = 1 + t^2, s = sinh x, r =
// sqrt(1 + s^2), tanh z = (b r s + i t) / (1 + b s^2); past (P + 4) ln 2 / 2,
// where tanh x is +-1 in the type, +-1 + i 4 sin y cos y e^(-2|x|). Annex G's
// values (G.6.2.6, as C17 has them, its DR 471): it is odd, conj(tanh z) =
// tanh(conj z), +inf + iy gives 1 + i0 sin 2y, +-0 + i(inf or NaN) +-0 +
// iNaN.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_tanh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();

    // x infinite: +-1, and a zero signed by sin 2y, or +-0
    if (d_internal_cplx_@S@_is_inf(x))
    {
        const @T@ one = (x < (@T@)0) ? (@T@)-1 : (@T@)1;

        if ( (y != y) ||
             (d_internal_cplx_@S@_is_inf(y)) )
        {
            return d_internal_cplx_@S@_make(one, (@T@)0);
        }

        return d_internal_cplx_@S@_make(
            one,
            d_internal_cplx_@S@_copysign(
                (@T@)0,
                d_math_@S@_sin@R@(y) * d_math_@S@_cos@R@(y)));
    }

    // y infinite or NaN, x finite: +-0 + iNaN for x 0, else NaNs
    if ( (y != y) ||
         (d_internal_cplx_@S@_is_inf(y)) )
    {
        if (x == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, nan);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // x NaN: NaN + i0 for y = 0, else NaNs
    if (x != x)
    {
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // y = 0: tanh x and y's zero
    if (y == (@T@)0)
    {
        return d_internal_cplx_@S@_make(d_math_@S@_tanh@R@(x), y);
    }

    // far from 0: +-1 + i 4 sin y cos y e^(-2|x|)
    if (d_internal_cplx_@S@_abs(x) > ((@T@)(@P@ + 4) * (@T@)0.3466L))
    {
        const @T@ e = d_math_@S@_exp@R@((@T@)-2 * d_internal_cplx_@S@_abs(x));

        return d_internal_cplx_@S@_make(
            (x < (@T@)0) ? (@T@)-1 : (@T@)1,
            (((@T@)4 * d_math_@S@_sin@R@(y)) * d_math_@S@_cos@R@(y)) * e);
    }

    {
        const @T@ t     = d_math_@S@_tan@R@(y);
        const @T@ beta  = (@T@)1 + (t * t);
        const @T@ s     = d_math_@S@_sinh@R@(x);
        const @T@ rho   = d_math_@S@_sqrt@R@((@T@)1 + (s * s));
        const @T@ denom = (@T@)1 + ((beta * s) * s);

        return d_internal_cplx_@S@_make(((beta * rho) * s) / denom,
                                        t / denom);
    }
}
"""

CPLX_INVERSE = r"""
// d_internal_cplx_@S@_half_excess@KS@
//   f(_a, _b) of Hull, Fairgrieve and Tang, with _h = hypot(_a, _b): half of
// _h - _b, written so that it never subtracts nearly equal values: (_h - _b)
// / 2 for _b < 0, _a / 2 for _b = 0, and _a^2 / (_h + _b) / 2 otherwise.
@KQ@ @T@
d_internal_cplx_@S@_half_excess@KS@(
    @T@ _a,
    @T@ _b,
    @T@ _h
)
{
    if (_b < (@T@)0)
    {
        return (_h - _b) / (@T@)2;
    }

    if (_b == (@T@)0)
    {
        return _a / (@T@)2;
    }

    return ((_a * _a) / (_h + _b)) / (@T@)2;
}

// dj_internal_cplx_@S@_hard_work
//   struct: what dj_internal_cplx_@S@_hard_work@KS@ finds of z = x + iy, x
// and y nonnegative: the real part of asinh z (rx), and the imaginary
// part's two ways, B = y / A where it is usable, else the arguments of an
// atan2.
struct dj_internal_cplx_@S@_hard_work@KS@
{
    @T@  rx;           // Re asinh(x + iy) = log(A + sqrt(A^2 - 1))
    bool b_is_usable;  // whether asin(B) gives the imaginary part
    @T@  b;            // B = y / A
    @T@  sqrt_a2my2;   // sqrt(A^2 - y^2), for atan2(new_y, it)
    @T@  new_y;        // y, scaled with it
};

// d_internal_cplx_@S@_hard_work_of@KS@
//   Hull, Fairgrieve and Tang's A and B of x + iy, x and y nonnegative and
// finite, below 1/EPSILON: R = |z + i|, S = |z - i|, A = (R + S) / 2 and B =
// y / A, so that asinh z = log(A + sqrt(A^2 - 1)) + i asin B, with A - 1
// and A - y written to keep their accuracy where A is near 1 or y, as
// FreeBSD's catrig.c writes them: A below 10 takes log1p of A - 1, B past
// 0.6417 takes atan2 of y and sqrt(A^2 - y^2).
@KQ@ struct dj_internal_cplx_@S@_hard_work@KS@
d_internal_cplx_@S@_hard_work_of@KS@(
    @T@ _x,
    @T@ _y
)
{
    const @T@ eps          = @EPS@;
    const @T@ four_sqrt_min = d_internal_math_@S@_pow2(((@EMIN@ - 1) / 2) + 2);
    const @T@ r            = d_math_@S@_hypot@R@(_x, _y + (@T@)1);
    const @T@ s            = d_math_@S@_hypot@R@(_x, _y - (@T@)1);
    const @T@ a0           = (r + s) / (@T@)2;
    // A >= 1 in exact arithmetic; so it is made here
    const @T@ a            = (a0 < (@T@)1) ? (@T@)1 : a0;
    struct dj_internal_cplx_@S@_hard_work@KS@ result =
        { (@T@)0, false, (@T@)0, (@T@)0, (@T@)0 };

    // the real part
    if (a < (@T@)10)
    {
        if ( (_y == (@T@)1) &&
             (_x < ((eps * eps) / (@T@)128)) )
        {
            // A - 1 of order x / 2, A = 1 inexactly
            result.rx = d_math_@S@_sqrt@R@(_x);
        }
        else if (_x >= (eps * d_internal_cplx_@S@_abs(_y - (@T@)1)))
        {
            const @T@ am1 =
                d_internal_cplx_@S@_half_excess@KS@(_x, (@T@)1 + _y, r) +
                d_internal_cplx_@S@_half_excess@KS@(_x, (@T@)1 - _y, s);

            result.rx = d_math_@S@_log1p@R@(
                am1 + d_math_@S@_sqrt@R@(am1 * (a + (@T@)1)));
        }
        else if (_y < (@T@)1)
        {
            result.rx = _x / d_math_@S@_sqrt@R@(((@T@)1 - _y) *
                                                ((@T@)1 + _y));
        }
        else
        {
            result.rx = d_math_@S@_log1p@R@(
                (_y - (@T@)1) +
                d_math_@S@_sqrt@R@((_y - (@T@)1) * (_y + (@T@)1)));
        }
    }
    else
    {
        result.rx = d_math_@S@_log@R@(
            a + d_math_@S@_sqrt@R@((a * a) - (@T@)1));
    }

    result.new_y = _y;

    // y tiny: y / A would underflow; atan2 of both scaled up instead
    if (_y < four_sqrt_min)
    {
        result.b_is_usable = false;
        result.sqrt_a2my2  = a * ((@T@)2 / eps);
        result.new_y       = _y * ((@T@)2 / eps);

        return result;
    }

    result.b           = _y / a;
    result.b_is_usable = true;

    // B near 1: asin loses its accuracy; atan2 instead
    if (result.b > (@T@)0.6417)
    {
        result.b_is_usable = false;

        if ( (_y == (@T@)1) &&
             (_x < (eps / (@T@)128)) )
        {
            result.sqrt_a2my2 = d_math_@S@_sqrt@R@(_x) *
                                d_math_@S@_sqrt@R@((a + _y) / (@T@)2);
        }
        else if (_x >= (eps * d_internal_cplx_@S@_abs(_y - (@T@)1)))
        {
            const @T@ amy =
                d_internal_cplx_@S@_half_excess@KS@(_x, _y + (@T@)1, r) +
                d_internal_cplx_@S@_half_excess@KS@(_x, _y - (@T@)1, s);

            result.sqrt_a2my2 = d_math_@S@_sqrt@R@(amy * (a + _y));
        }
        else if (_y > (@T@)1)
        {
            const @T@ lift = (@T@)4 / (eps * eps);

            result.sqrt_a2my2 = ((_x * lift) * _y) /
                                d_math_@S@_sqrt@R@((_y + (@T@)1) *
                                                   (_y - (@T@)1));
            result.new_y      = _y * lift;
        }
        else
        {
            result.sqrt_a2my2 = d_math_@S@_sqrt@R@(((@T@)1 - _y) *
                                                   ((@T@)1 + _y));
        }
    }

    return result;
}

// d_internal_cplx_@S@_log_large@KS@
//   log _z for |_z| past 1/EPSILON and finite, without hypot's overflow:
// past half the range both parts halved and ln 2 added, as for log |z|.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log_large@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_log_modulus@KS@(_z.part[0], _z.part[1]),
        d_math_@S@_atan2@R@(_z.part[1], _z.part[0]));
}

// d_internal_cplx_@S@_asinh@KS@
//   asinh _z: Hull, Fairgrieve and Tang's algorithm, as FreeBSD's catrig.c
// has it, on |x| and |y|, the signs put back: past 1/EPSILON log(2z) by the
// logarithm of large values; below sqrt(6 EPSILON) / 4 in both parts z
// itself; and Annex G's values (G.6.2.2) at the infinities and NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_asinh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ ax  = d_internal_cplx_@S@_abs(x);
    const @T@ ay  = d_internal_cplx_@S@_abs(y);
    const @T@ nan = d_internal_math_@S@_nan();
    const @T@ recip_eps = (@T@)1 / @EPS@;

    // a NaN part
    if ( (x != x) ||
         (y != y) )
    {
        // +-inf + iNaN: +-inf + iNaN; NaN + i(+-inf): +-inf + iNaN
        if (d_internal_cplx_@S@_is_inf(x))
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        if (d_internal_cplx_@S@_is_inf(y))
        {
            return d_internal_cplx_@S@_make(y, x);
        }

        // NaN + i0: NaN + i0
        if (y == (@T@)0)
        {
            return d_internal_cplx_@S@_make(x, y);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // far from 0, infinities among it: log(2|z|) + i arg, signed back
    if ( (ax > recip_eps) ||
         (ay > recip_eps) )
    {
        const struct dj_cplx_@S@ w = d_internal_cplx_@S@_log_large@KS@(
            d_internal_math_@S@_negative(x) ? d_cplx_@S@_neg(_z) : _z);

        return d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_copysign(
                w.part[0] + (@T@)0.693147180559945309417232121458176568L,
                x),
            d_internal_cplx_@S@_copysign(w.part[1], y));
    }

    // 0, and values whose next term lies below their last place
    if ( ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         ( (ax < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) &&
           (ay < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) ) )
    {
        return _z;
    }

    {
        const struct dj_internal_cplx_@S@_hard_work@KS@ h =
            d_internal_cplx_@S@_hard_work_of@KS@(ax, ay);
        const @T@ ry = h.b_is_usable
                           ? d_math_@S@_asin@R@(h.b)
                           : d_math_@S@_atan2@R@(h.new_y, h.sqrt_a2my2);

        return d_internal_cplx_@S@_make(d_internal_cplx_@S@_copysign(h.rx, x),
                                        d_internal_cplx_@S@_copysign(ry, y));
    }
}

// d_internal_cplx_@S@_acos@KS@
//   acos _z: Hull, Fairgrieve and Tang's algorithm on |y| and |x|, as
// FreeBSD's catrig.c has it: the real part from B or atan2, signed by x, the
// imaginary part -sign(y) Re asinh; and Annex G's values (G.6.1.1).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_acos@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x    = _z.part[0];
    const @T@ y    = _z.part[1];
    const @T@ ax   = d_internal_cplx_@S@_abs(x);
    const @T@ ay   = d_internal_cplx_@S@_abs(y);
    const bool sx  = d_internal_math_@S@_negative(x);
    const bool sy  = d_internal_math_@S@_negative(y);
    const @T@ nan  = d_internal_math_@S@_nan();
    const @T@ pio2 = (@T@)1.570796326794896619231321691639751442L;
    const @T@ recip_eps = (@T@)1 / @EPS@;

    // a NaN part
    if ( (x != x) ||
         (y != y) )
    {
        // +-inf + iNaN: NaN +- i inf
        if (d_internal_cplx_@S@_is_inf(x))
        {
            return d_internal_cplx_@S@_make(nan,
                                            -d_internal_math_@S@_infinity());
        }

        // NaN + i(+-inf): NaN -+ i inf
        if (d_internal_cplx_@S@_is_inf(y))
        {
            return d_internal_cplx_@S@_make(nan, -y);
        }

        // +-0 + iNaN: pi/2 + iNaN
        if (x == (@T@)0)
        {
            return d_internal_cplx_@S@_make(pio2, nan);
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // far from 0, infinities among it
    if ( (ax > recip_eps) ||
         (ay > recip_eps) )
    {
        const struct dj_cplx_@S@ w = d_internal_cplx_@S@_log_large@KS@(_z);
        const @T@ rx = d_internal_cplx_@S@_abs(w.part[1]);
        const @T@ ry = w.part[0] +
                       (@T@)0.693147180559945309417232121458176568L;

        return d_internal_cplx_@S@_make(rx, sy ? ry : -ry);
    }

    // 1 + i0: 0 - i0
    if ( (x == (@T@)1) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make((@T@)0, -y);
    }

    // near 0: pi/2 - x - iy
    if ( (ax < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) &&
         (ay < (d_math_@S@_sqrt_c((@T@)6 * @EPS@) / (@T@)4)) )
    {
        return d_internal_cplx_@S@_make(pio2 - x, -y);
    }

    {
        const struct dj_internal_cplx_@S@_hard_work@KS@ h =
            d_internal_cplx_@S@_hard_work_of@KS@(ay, ax);
        const @T@ rx = h.b_is_usable
                           ? d_math_@S@_acos@R@(sx ? -h.b : h.b)
                           : d_math_@S@_atan2@R@(h.sqrt_a2my2,
                                                 sx ? -h.new_y : h.new_y);

        return d_internal_cplx_@S@_make(rx, sy ? h.rx : -h.rx);
    }
}

// d_internal_cplx_@S@_acosh@KS@
//   acosh _z from acos _z, as FreeBSD's catrig.c takes it: i acos z or
// -i acos z, whichever has a nonnegative real part, and Annex G's values
// (G.6.2.1) where a part is a NaN.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_acosh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const struct dj_cplx_@S@ w = d_internal_cplx_@S@_acos@KS@(_z);
    const @T@ rx = w.part[0];
    const @T@ ry = w.part[1];

    // NaNs: NaNs
    if ( (rx != rx) &&
         (ry != ry) )
    {
        return d_internal_cplx_@S@_make(ry, rx);
    }

    // an infinity and a NaN: +inf + iNaN
    if (rx != rx)
    {
        return d_internal_cplx_@S@_make(d_internal_cplx_@S@_abs(ry), rx);
    }

    // +-0 + iNaN, whose acos is pi/2 + iNaN: NaN + i pi/2, as C17 has it
    // (its DR 471), where C11 and FreeBSD's give NaNs
    if (ry != ry)
    {
        return d_internal_cplx_@S@_make(ry, rx);
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_abs(ry),
        d_internal_cplx_@S@_copysign(rx, _z.part[1]));
}

// d_internal_cplx_@S@_one_less_squares
//   1 - _a^2 - _b^2, _a and _b nonnegative and finite: the squares exact,
// their high parts summed with -1 exactly, so that the cancellation near
// the unit circle loses nothing but the last rounding.
D_MATH_FN @T@
d_internal_cplx_@S@_one_less_squares@KS@(
    @T@ _a,
    @T@ _b
)
{
#if @WIDE@
    // as long double pairs where @T@ is evaluated wider (section 2), the sum
    // rounded once
    const struct dj_internal_math_ld_pair aa =
        d_internal_math_ld_product_exact((long double)_a, (long double)_a);
    const struct dj_internal_math_ld_pair bb =
        d_internal_math_ld_product_exact((long double)_b, (long double)_b);
    const struct dj_internal_math_ld_pair t1 =
        d_internal_math_ld_two_sum(1.0L, -aa.high);
    const struct dj_internal_math_ld_pair t2 =
        d_internal_math_ld_two_sum(t1.high, -bb.high);
    const struct dj_internal_math_ld_pair v =
        d_internal_math_ld_two_sum(t2.high,
                                   (t1.low + t2.low) - (aa.low + bb.low));

    return d_internal_math_@S@_round_pair(v.high, v.low);
#else  // @WIDE@
    const struct dj_internal_math_@S@_pair aa =
        d_internal_cplx_@S@_product(_a, _a);
    const struct dj_internal_math_@S@_pair bb =
        d_internal_cplx_@S@_product(_b, _b);
    const struct dj_internal_math_@S@_pair t1 =
        d_internal_cplx_@S@_two_sum((@T@)1, -aa.high);
    const struct dj_internal_math_@S@_pair t2 =
        d_internal_cplx_@S@_two_sum(t1.high, -bb.high);

    return t2.high + ((t1.low + t2.low) - (aa.low + bb.low));
#endif  // @WIDE@
}

// d_internal_cplx_@S@_atanh@KS@
//   atanh _z: as FreeBSD's catrig.c has it, Kahan's formulas - Re =
// log1p(4|x| / ((|x| - 1)^2 + y^2)) / 4, Im = atan2(2|y|, 1 - x^2 - y^2) / 2
// - the second's 1 - x^2 - y^2 here taken exactly; far from 0, Re (1/z) +
// i(+-pi/2), by the quotient; near 0, z itself; and Annex G's values
// (G.6.2.3) at the infinities and NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_atanh@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x    = _z.part[0];
    const @T@ y    = _z.part[1];
    const @T@ ax   = d_internal_cplx_@S@_abs(x);
    const @T@ ay   = d_internal_cplx_@S@_abs(y);
    const @T@ nan  = d_internal_math_@S@_nan();
    const @T@ pio2 = (@T@)1.570796326794896619231321691639751442L;
    const @T@ ln2  = (@T@)0.693147180559945309417232121458176568L;
    const @T@ eps  = @EPS@;
    const @T@ recip_eps = (@T@)1 / @EPS@;

    // the real segment [-1, 1]: the real atanh, y's zero
    if ( (y == (@T@)0) &&
         (ax <= (@T@)1) )
    {
        return d_internal_cplx_@S@_make(d_math_@S@_atanh@R@(x), y);
    }

    // the imaginary axis: x's zero and atan y, as accurate as atan
    if (x == (@T@)0)
    {
        return d_internal_cplx_@S@_make(x, d_math_@S@_atan@R@(y));
    }

    // a NaN part
    if ( (x != x) ||
         (y != y) )
    {
        // +-inf + iNaN: +-0 + iNaN
        if (d_internal_cplx_@S@_is_inf(x))
        {
            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_copysign((@T@)0, x),
                y);
        }

        // NaN + i(+-inf): +-0 + i(+-pi/2)
        if (d_internal_cplx_@S@_is_inf(y))
        {
            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_copysign((@T@)0, x),
                d_internal_cplx_@S@_copysign(pio2, y));
        }

        return d_internal_cplx_@S@_make(nan, nan);
    }

    // far from 0, infinities among it: Re(1/z) + i(+-pi/2)
    if ( (ax > recip_eps) ||
         (ay > recip_eps) )
    {
        const struct dj_cplx_@S@ q =
            d_cplx_@S@_div(d_internal_cplx_@S@_make((@T@)1, (@T@)0), _z);

        return d_internal_cplx_@S@_make(
            d_internal_cplx_@S@_copysign(q.part[0], x),
            d_internal_cplx_@S@_copysign(pio2, y));
    }

    // near 0: z itself
    if ( (ax < (d_math_@S@_sqrt_c((@T@)3 * eps) / (@T@)2)) &&
         (ay < (d_math_@S@_sqrt_c((@T@)3 * eps) / (@T@)2)) )
    {
        return _z;
    }

    {
        @T@ rx = (@T@)0;
        @T@ ry = (@T@)0;

        // the real part: near |x| = 1 and y = 0 its log form
        if ( (ax == (@T@)1) &&
             (ay < eps) )
        {
            rx = (ln2 - d_math_@S@_log@R@(ay)) / (@T@)2;
        }
        else
        {
            const @T@ xm1  = ax - (@T@)1;
            const @T@ sum2 = (ay < d_internal_math_@S@_pow2((@EMIN@ - 1) / 2))
                                 ? (xm1 * xm1)
                                 : ((xm1 * xm1) + (ay * ay));

            rx = d_math_@S@_log1p@R@(((@T@)4 * ax) / sum2) / (@T@)4;
        }

        // the imaginary part
        if (ax == (@T@)1)
        {
            ry = d_math_@S@_atan2@R@((@T@)2, -ay) / (@T@)2;
        }
        else
        {
            ry = d_math_@S@_atan2@R@(
                     (@T@)2 * ay,
                     d_internal_cplx_@S@_one_less_squares@KS@(ax, ay)) /
                 (@T@)2;
        }

        return d_internal_cplx_@S@_make(d_internal_cplx_@S@_copysign(rx, x),
                                        d_internal_cplx_@S@_copysign(ry, y));
    }
}
"""

# compositions: the trigonometric functions by the hyperbolic ones, asin and
# atan by asinh and atanh, as Annex G defines them
CPLX_COMPOSED = r"""
// d_internal_cplx_@S@_sin@KS@, d_internal_cplx_@S@_cos@KS@,
// d_internal_cplx_@S@_tan@KS@
//   sin z = -i sinh(iz), cos z = cosh(iz), tan z = -i tanh(iz): Annex G's
// definitions (G.6, its first paragraph), so that their special values are
// the hyperbolic functions' turned.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_sin@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_unturned(
        d_internal_cplx_@S@_sinh@KS@(d_internal_cplx_@S@_turned(_z)));
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_cos@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_cosh@KS@(d_internal_cplx_@S@_turned(_z));
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_tan@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_unturned(
        d_internal_cplx_@S@_tanh@KS@(d_internal_cplx_@S@_turned(_z)));
}

// d_internal_cplx_@S@_asin@KS@, d_internal_cplx_@S@_atan@KS@
//   asin z = -i asinh(iz) and atan z = -i atanh(iz), each the swapped
// function of the swapped argument (Annex G, G.6).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_asin@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_swapped(
        d_internal_cplx_@S@_asinh@KS@(d_internal_cplx_@S@_swapped(_z)));
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_atan@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_swapped(
        d_internal_cplx_@S@_atanh@KS@(d_internal_cplx_@S@_swapped(_z)));
}
"""

# the turns, once per family with the arithmetic
CPLX_TURNS = r"""
// d_internal_cplx_@S@_turned
//   i _z's coordinates: (-y, x), so that sin z = -i sinh(iz), cos z =
// cosh(iz) and tan z = -i tanh(iz) take the hyperbolic kernels.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_turned(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(-_z.part[1], _z.part[0]);
}

// d_internal_cplx_@S@_unturned
//   -i _z's coordinates: (y, -x).
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_unturned(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_z.part[1], -_z.part[0]);
}

// d_internal_cplx_@S@_swapped
//   (y, x): asin and atan by asinh and atanh, as Hull, Fairgrieve and Tang
// and FreeBSD's catrig.c take them.
D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_swapped(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_make(_z.part[1], _z.part[0]);
}
"""
