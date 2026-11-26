"""gen_c_math.elementary - the text of elementary.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (DATE, FAMILIES, banner, instantiate, overload_section,
                     re)


# -----------------------------------------------------------------------------
# elementary.h
# -----------------------------------------------------------------------------
# The sine's and cosine's kernels and the arc-tangent's, with the exact sums
# and products that exponential.h's and tangent.h's kernels share, are written
# once, in long double (the header's section 1); each family's square root,
# arc-sine, arc-cosine, sine and cosine follow.

# 2/pi to 704 digits of 24 bits, the reduction's table: computed here, by
# Machin's formula in Python's integers, rather than pasted in, so that the
# header's digits can be traced to their source (the slice's tools check them
# against mpmath).
TWO_OVER_PI_DIGITS = 704


def two_over_pi_bits(count):
    """floor(2/pi 2^count), from pi to count + 64 bits by Machin's formula,
    pi = 16 atan(1/5) - 4 atan(1/239), each series summed in integers with 64
    guard bits; the floor is taken at both ends of pi's error, which must
    agree."""
    bits = count + 64
    guard = 64
    one = 1 << (bits + guard)

    def atan_inv(n):
        total = term = one // n
        k, sign = 1, 1
        while term:
            term //= n * n
            k += 2
            sign = -sign
            total += sign * (term // k)
        return total

    pi = (16 * atan_inv(5) - 4 * atan_inv(239)) >> guard
    low = (1 << (count + 1 + bits)) // (pi + 2)
    high = (1 << (count + 1 + bits)) // (pi - 2)
    assert low == high, "2/pi: a run of equal bits at the cut"
    return low


def two_over_pi_literal():
    """the table as a C string literal, three bytes a digit, 16 bytes a
    line, highest first."""
    data = two_over_pi_bits(24 * TWO_OVER_PI_DIGITS).to_bytes(
        3 * TWO_OVER_PI_DIGITS, "big")
    rows = []
    for i in range(0, len(data), 16):
        rows.append('        "' +
                    "".join("\\x%02X" % b for b in data[i:i + 16]) + '"')
    return "\n".join(rows)


ELEMENTARY_KERNELS = r"""
//==============================================================================
// 1.  THE KERNELS (long double)
//==============================================================================
// What every family's sine, cosine, arc-sine and arc-cosine - and
// tangent.h's tangent and arc-tangents - are computed with, once, in long
// double: exact sums and products (which exponential.h's kernels use too),
// 2/pi to as many bits as the widest long double reaches, the reduction of an
// argument by its nearest multiple of pi/2, the sine and cosine of what
// remains, the arc-tangent of the unit interval, and the arc-sine and
// arc-cosine on it; each result kept as an exact pair - a value and the
// rounding error below it - so that a family rounds it once. Nothing depends
// on long double's format beyond <float.h>: the text is the same for an x87's
// 64 bits, for a long double that is double, and for a quad's 113 bits.

// the reduction reaches a long double of at most 120 bits whose values lie
// below 2^16384: an x87's, a quad's, and double's
#if ( (LDBL_MANT_DIG > 120) ||                                                \
      (LDBL_MAX_EXP > 16384) )
    #error "elementary.h: no reduction by pi/2 for so wide a long double"
#endif


// 1.1    Exact sums and products
//------------------------------------------------------------------------------

// dj_internal_math_ld_pair
//   struct: a value as an unevaluated sum, high + low, the low part below the
// high part's rounding.
struct dj_internal_math_ld_pair
{
    long double high;
    long double low;
};

// d_internal_math_ld_two_sum
//   _a + _b exactly, as high + low: Knuth's two-sum, which needs no order
// between the operands' magnitudes.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_two_sum(
    long double _a,
    long double _b
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };
    long double                     a_part = 0.0L;
    long double                     b_part = 0.0L;

    result.high = _a + _b;
    a_part      = result.high - _b;
    b_part      = result.high - a_part;
    result.low  = (_a - a_part) + (_b - b_part);

    return result;
}

// d_internal_math_ld_product_exact
//   _a * _b exactly, as high + low: the low part by a fused multiply-add
// where the target has one for long double, and otherwise by Dekker's
// product, each factor split by Veltkamp's constant 2^ceil(P/2) + 1, P long
// double's precision (float_order.h); both exact, so both give the same bits
// (math_common.h, section 5). The factors must be normal (or zero) and their
// product far from overflow.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_product_exact(
    long double _a,
    long double _b
)
{
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

#if D_MATH_LDBL_FMA
    if (D_MATH_RUN_TIME())
    {
        result.high = _a * _b;
        result.low  = D_MATH_LDBL_FMA_OF(_a, _b, -result.high);

        return result;
    }
#endif  // D_MATH_LDBL_FMA

    const long double split = d_internal_math_ld_veltkamp();
    const long double ta    = split * _a;
    const long double a_hi  = ta - (ta - _a);
    const long double a_lo  = _a - a_hi;
    const long double tb    = split * _b;
    const long double b_hi  = tb - (tb - _b);
    const long double b_lo  = _b - b_hi;

    result.high = _a * _b;
    result.low  = ( (((a_hi * b_hi) - result.high) + (a_hi * b_lo)) +
                    (a_lo * b_hi) ) +
                  (a_lo * b_lo);

    return result;
}

// 1.2    The digits of 2/pi
//------------------------------------------------------------------------------
// An argument past 2^20 quarter turns is reduced with 2/pi's bits from its
// exponent's place to 480 past it, the reduction's columns (section 1.3):
// for the largest value of a 15-bit exponent, an x87's or a quad's, 2/pi's
// first 16,848 bits. Here are @DIGITS@ digits of 24 bits, @BITS@ bits, as a
// string literal, three bytes a digit: a literal's characters can be read in
// a constant expression with no constexpr object to declare, are never copied
// onto the stack, cost nothing to a translation unit that never reaches them,
// and have no linkage for C and C++ to disagree on. The generator computes
// them, by Machin's formula.

// d_internal_math_ld_two_over_pi_digit
//   2/pi's digit _index, base 2^24, from the binary point: 2/pi = the sum of
// digit(j) 2^(-24 (j + 1)) over j >= 0. 0 for a negative _index, 2/pi having
// no integer part.
D_MATH_FN long double
d_internal_math_ld_two_over_pi_digit(
    int _index
)
{
    // 2/pi's first @BITS@ bits, three bytes a digit, highest first
    const char* const bytes =
@TWO_OVER_PI@;

    if ( (_index < 0) ||
         (_index >= @DIGITS@) )
    {
        return 0.0L;
    }

    return ( ((long double)(unsigned char)bytes[3 * _index] * 65536.0L) +
             ((long double)(unsigned char)bytes[(3 * _index) + 1] * 256.0L) ) +
           (long double)(unsigned char)bytes[(3 * _index) + 2];
}

// 1.3    Reduction by pi/2
//------------------------------------------------------------------------------
// x = n pi/2 + r, n the integer nearest x 2/pi and |r| <= pi/4, r carried as
// r + r_lo. Below 2^20 quarter turns, Cody and Waite's: pi/2 in pieces of at
// most 32 bits, written exactly, so that each product with n is exact in any
// format of 53 bits or more, and each difference's error kept. Past them, and
// within 2^-53 of a multiple of pi/2, where those pieces would not reach,
// Payne and Hanek's: x's digits, base 2^24, times as many of 2/pi's as x's
// exponent reaches, in exact columns of 48-bit products; the integer part kept
// only mod 4, the fraction from its first nonzero digit - the digits before
// it are what x's nearness to a multiple of pi/2 cancelled - to past twice
// long double's precision, and then multiplied by pi/2. Either way r is
// within 2^-(P+30) of itself, P long double's precision, at every finite x.

// dj_internal_math_ld_reduced
//   struct: an argument reduced by pi/2: x = n pi/2 + (r + r_lo).
struct dj_internal_math_ld_reduced
{
    long double r;         // |r| <= pi/4, rounded once
    long double r_lo;      // the rest, below r's last place
    int         quadrant;  // n mod 4, in [0, 4)
};

// d_internal_math_ld_floor_small
//   the greatest integer at most _v, for 0 <= _v < 2^(P-2): _v rounded to an
// integer by adding and subtracting 1.5 2^(P-1) (LDBL_EPSILON being 2^(1-P)),
// then lowered if that rounded up.
D_MATH_FN long double
d_internal_math_ld_floor_small(
    long double _v
)
{
    const long double magic = 1.5L / LDBL_EPSILON;
    const long double whole = (_v + magic) - magic;

    return (whole > _v) ? (whole - 1.0L)
                        : whole;
}

// d_internal_math_ld_reduce_far
//   _x reduced by pi/2 by Payne and Hanek's method, for |_x| >= 1 and finite.
D_MATH_FN struct dj_internal_math_ld_reduced
d_internal_math_ld_reduce_far(
    long double _x
)
{
    const long double radix   = 16777216.0L;
    const long double ax      = (_x < 0.0L) ? -_x : _x;
    const struct dj_internal_math_ld_binade binade =
        d_internal_math_ld_binade_of(ax);
    // 2^(24 a) <= |x| < 2^(24 (a + 1))
    const int         a       = binade.exponent / 24;
    // pi/2 = pio2_hi + pio2_lo, pio2_hi of 31 bits: exact as written
    const long double pio2_hi = 1.570796326734125614166259765625L;
    const long double pio2_lo = 6.077100506506192601475144209858469968755e-11L;
    long double       digits[6]   = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
    long double       columns[21] = { 0.0L };
    long double       y           = ax * d_internal_math_ld_pow2(-24 * a);
    long double       quadrant    = 0.0L;
    bool              negative    = false;
    int               count       = 0;
    int               first       = 1;
    int               i           = 0;
    int               c           = 0;
    struct dj_internal_math_ld_reduced result = { 0.0L, 0.0L, 0 };

    // x's digits, base 2^24, from 2^(24 a) down: six reach 120 bits
    for (i = 0; ( (i < 6) && (y != 0.0L) ); ++i)
    {
        digits[i] = d_internal_math_ld_floor_small(y);
        y         = (y - digits[i]) * radix;
        count     = i + 1;
    }

    // x 2/pi in columns of weight 2^(-24 c): x's digit i times 2/pi's digit
    // j lands in column i + j + 1 - a, and the columns of weight 2^24 and up,
    // multiples of 4, are left out. A product is below 2^48 and a column below
    // 2^51: every sum exact
    for (c = 0; c <= 20; ++c)
    {
        long double sum = 0.0L;

        for (i = 0; i < count; ++i)
        {
            sum += digits[i] *
                   d_internal_math_ld_two_over_pi_digit(c + a - 1 - i);
        }

        columns[c] = sum;
    }

    // the carries, from the last column up, leave each fraction column a
    // digit
    for (c = 20; c >= 1; --c)
    {
        const long double carry =
            d_internal_math_ld_floor_small(columns[c] / radix);

        columns[c]     -= carry * radix;
        columns[c - 1] += carry;
    }

    // the integer part, mod 4
    quadrant = columns[0] -
               (4.0L * d_internal_math_ld_floor_small(columns[0] / 4.0L));

    // to the nearest integer: a fraction of a half or more is one more turn
    // and, below it, 1 - the fraction, its digits complemented
    if (columns[1] >= (radix / 2.0L))
    {
        negative = true;
        quadrant = (quadrant >= 3.0L) ? 0.0L
                                      : (quadrant + 1.0L);

        for (c = 1; c <= 20; ++c)
        {
            columns[c] = (radix - 1.0L) - columns[c];
        }

        columns[20] += 1.0L;

        for (c = 20; ( (c >= 2) && (columns[c] >= radix) ); --c)
        {
            columns[c]     -= radix;
            columns[c - 1] += 1.0L;
        }
    }

    // the fraction's first nonzero digit
    while ( (first < 20) &&
            (columns[first] == 0.0L) )
    {
        ++first;
    }

    {
        // the fraction as hi + lo, from that digit to past twice long
        // double's precision (fast two-sums: each digit is below the sum)
        const int   last  = ((first + ((2 * LDBL_MANT_DIG) / 24) + 2) < 20)
                                ? (first + ((2 * LDBL_MANT_DIG) / 24) + 2)
                                : 20;
        long double scale = d_internal_math_ld_pow2(-24 * first);
        long double hi    = 0.0L;
        long double lo    = 0.0L;

        for (c = first; c <= last; ++c)
        {
            const long double term = columns[c] * scale;
            const long double sum  = hi + term;

            lo    += term - (sum - hi);
            hi     = sum;
            scale /= radix;
        }

        {
            // renormalized, then times pi/2: f_hi pio2_hi exactly, the rest
            // rounded below it
            const long double f_hi = hi + lo;
            const long double f_lo = lo - (f_hi - hi);
            const struct dj_internal_math_ld_pair head =
                d_internal_math_ld_product_exact(f_hi, pio2_hi);
            const long double rest = head.low +
                                     ((f_hi * pio2_lo) + (f_lo * pio2_hi));

            result.r    = head.high + rest;
            result.r_lo = rest - (result.r - head.high);
        }
    }

    // the signs: the fraction's, then x's, -x being (-n) pi/2 - r
    if (negative != (_x < 0.0L))
    {
        result.r    = -result.r;
        result.r_lo = -result.r_lo;
    }

    if ( (_x < 0.0L) &&
         (quadrant != 0.0L) )
    {
        quadrant = 4.0L - quadrant;
    }

    result.quadrant = (int)quadrant;

    return result;
}

// d_internal_math_ld_reduce
//   _x reduced by pi/2, _x finite: Cody and Waite's way below 2^20 quarter
// turns, Payne and Hanek's past them and within 2^-53 of a multiple of pi/2.
D_MATH_FN struct dj_internal_math_ld_reduced
d_internal_math_ld_reduce(
    long double _x
)
{
    const long double two_over_pi = 0.636619772367581343075535053490057448L;
    const long double turns       = _x * two_over_pi;

    if (!( (turns < 1048576.0L) &&
           (turns > -1048576.0L) ))
    {
        return d_internal_math_ld_reduce_far(_x);
    }

    {
        // pi/2 = pio2_1 + pio2_2 + pio2_3 + pio2_3t, the first three of 31,
        // 32 and 28 bits, written exactly: as decimals of 21 digits they round
        // to the same values in 53 and 64 bits, but not in a quad's 113
        const long double pio2_1   = 1.570796326734125614166259765625L;
        const long double pio2_2   = 2242054355.0L / 36893488147419103232.0L;
        const long double pio2_3   = 160220439.0L /
                                     79228162514264337593543950336.0L;
        const long double pio2_3t  =
            8.478427660368899643958701469386701878e-32L;
        // 1.5 2^(P-1): adding and subtracting it rounds turns to an integer
        const long double magic    = 1.5L / LDBL_EPSILON;
        // 2^-53
        const long double tiny     = 1.0L / 9007199254740992.0L;
        const long double n        = (turns + magic) - magic;
        // x - n pi/2: three exact products, each difference's error kept
        // (two-sum: the first operand is the larger)
        const long double a        = _x - (n * pio2_1);
        const long double b        = n * pio2_2;
        const long double r1       = a - b;
        const long double e1       = (a - r1) - b;
        const long double c        = n * pio2_3;
        const long double r2       = r1 - c;
        const long double e2       = (r1 - r2) - c;
        const long double low      = (e1 + e2) - (n * pio2_3t);
        // the quadrant, n mod 4
        const long double fourths  = n / 4.0L;
        const long double whole    = ((fourths + magic) - magic);
        long double       quadrant = n - (4.0L * whole);
        struct dj_internal_math_ld_reduced result = { 0.0L, 0.0L, 0 };

        // the rounding of n / 4 may land a quadrant of -1 or 4
        quadrant = (quadrant < 0.0L) ? (quadrant + 4.0L)
                                     : ((quadrant >= 4.0L) ? (quadrant - 4.0L)
                                                           : quadrant);

        result.r        = r2 + low;
        result.r_lo     = low - (result.r - r2);
        result.quadrant = (int)quadrant;

        // the pieces hold pi/2 to 2^-(103+P), so n pi/2 to 2^-(83+P), which
        // is 2^-(P+30) of r while |r| is 2^-53 or more; nearer a multiple of
        // pi/2 - in a quad, where the cancellation can pass 100 bits - x
        // takes the exact way
        if ( (n != 0.0L) &&
             (result.r < tiny) &&
             (result.r > -tiny) )
        {
            return d_internal_math_ld_reduce_far(_x);
        }

        return result;
    }
}

// 1.4    Sine and cosine of the reduced argument
//------------------------------------------------------------------------------
// sin r and cos r by their Taylor series on [-pi/4, pi/4], sixteen terms past
// a quad's precision, each summed with Kahan's compensation; r_lo enters at
// the end, sin(r + r_lo) = sin r + r_lo cos r and cos(r + r_lo) = cos r -
// r_lo sin r, and each result is kept as a pair, the compensation in its low
// part.

// dj_internal_math_ld_sin_cos
//   struct: the sine and cosine of an argument's remainder after its
// reduction by pi/2, and the quadrant that remainder lies in.
struct dj_internal_math_ld_sin_cos
{
    struct dj_internal_math_ld_pair sine;      // sin(r + r_lo)
    struct dj_internal_math_ld_pair cosine;    // cos(r + r_lo)
    int                             quadrant;  // n mod 4
};

// d_internal_math_ld_sin_cos_of
//   _x reduced by pi/2, and the sine and cosine of what remains; _x finite.
D_MATH_FN struct dj_internal_math_ld_sin_cos
d_internal_math_ld_sin_cos_of(
    long double _x
)
{
    const struct dj_internal_math_ld_reduced reduced =
        d_internal_math_ld_reduce(_x);
    const long double r        = reduced.r;
    const long double r2sq     = r * r;
    long double       sin_sum  = r;
    long double       sin_comp = 0.0L;
    long double       cos_sum  = 1.0L;
    long double       cos_comp = 0.0L;
    long double       sin_term = r;
    long double       cos_term = 1.0L;
    int               k        = 1;
    struct dj_internal_math_ld_sin_cos result =
        { { 0.0L, 0.0L }, { 0.0L, 0.0L }, 0 };

    for (k = 1; k < 17; ++k)
    {
        sin_term = -sin_term * r2sq /
                   (long double)((2 * k) * ((2 * k) + 1));
        cos_term = -cos_term * r2sq /
                   (long double)(((2 * k) - 1) * (2 * k));

        {
            const long double ys = sin_term - sin_comp;
            const long double ts = sin_sum + ys;
            const long double yc = cos_term - cos_comp;
            const long double tc = cos_sum + yc;

            sin_comp = (ts - sin_sum) - ys;
            sin_sum  = ts;
            cos_comp = (tc - cos_sum) - yc;
            cos_sum  = tc;
        }
    }

    // the sums, with r_lo's terms and each compensation (Kahan's sum is the
    // sum less its compensation) below them
    result.sine     = d_internal_math_ld_two_sum(
                          sin_sum, (reduced.r_lo * cos_sum) - sin_comp);
    result.cosine   = d_internal_math_ld_two_sum(
                          cos_sum, -(reduced.r_lo * sin_sum) - cos_comp);
    result.quadrant = reduced.quadrant;

    return result;
}

// d_internal_math_ld_trig
//   sin(_x) when _cosine is false, cos(_x) when it is true, as high + low;
// _x finite.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_trig(
    long double _x,
    bool        _cosine
)
{
    const struct dj_internal_math_ld_sin_cos sc =
        d_internal_math_ld_sin_cos_of(_x);
    // cos x = sin(x + pi/2): a quarter turn more
    const int quadrant = _cosine ? ((sc.quadrant + 1) % 4)
                                 : sc.quadrant;
    // sin r, cos r, -sin r, -cos r, by quadrant
    struct dj_internal_math_ld_pair result =
        ((quadrant % 2) == 0) ? sc.sine : sc.cosine;

    if (quadrant >= 2)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// 1.5    The arc-tangent of the unit interval
//------------------------------------------------------------------------------
// q pi/2 + atan u or q pi/2 - atan u, for u in [0, 1] given as a pair and q
// in {0, 1, 2}: what every arc-tangent, arc-sine and arc-cosine comes to once
// its argument's signs are taken off and the larger magnitude divided into
// the smaller (tangent.h's atan and atan2, section 1.6's asin and acos). u is
// reduced by the nearest of 0, 1/4, 1/2, 3/4 and 1, c: atan u = atan c +
// atan t, t = (u - c) / (1 + c u), |t| <= 1/8, carried as t + t_lo (u - c
// exact by Sterbenz's lemma, the quotient's residual by Dekker's product).
// atan t = t - t^3/3 + t^5/5 - ..., to its 24th term, past a quad's
// precision; atan c and pi/2 are each held as a head of at most 32 bits,
// exact as written, and the rest; and the heads and t are summed exactly -
// q pi/2 + atan c is exact for each q - so that only the last addition
// rounds, and what it rounds off is kept.

// d_internal_math_ld_ratio
//   (_n_hi + _n_lo) / (_d_hi + _d_lo) as high + low, for 0 <= _n <= _d,
// _d > 0 and finite: the quotient with its residual, the residual's first
// product taken on copies scaled by _d's binade into [1, 2), where Dekker's
// product can neither overflow nor lose its low part. Below 2^(2P) of the
// normal range's bottom the quotient goes alone: its own rounding is then
// the result's.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_ratio(
    long double _n_hi,
    long double _n_lo,
    long double _d_hi,
    long double _d_lo
)
{
    // a subnormal _d is first lifted, with _n, into the normal range
    const long double lift = (_d_hi < LDBL_MIN)
                                 ? d_internal_math_ld_pow2(LDBL_MANT_DIG)
                                 : 1.0L;
    const long double n    = _n_hi * lift;
    const long double d    = _d_hi * lift;
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    result.high = n / d;

    if (result.high >= d_internal_math_ld_pow2(LDBL_MIN_EXP +
                                               (2 * LDBL_MANT_DIG)))
    {
        const struct dj_internal_math_ld_binade binade =
            d_internal_math_ld_binade_of(d);
        const long double scale = d_internal_math_ld_pow2(-binade.exponent);
        const long double n2    = n * scale;
        const long double d2    = d * scale;
        const struct dj_internal_math_ld_pair p =
            d_internal_math_ld_product_exact(result.high, d2);

        // n - q d: the highs' part exact, the lows' below it
        result.low = (((n2 - p.high) - p.low) / d2) +
                     ((_n_lo - (result.high * _d_lo)) / _d_hi);
    }

    return result;
}

// d_internal_math_ld_atan_reduced
//   _quarters pi/2 + atan(_u), or _quarters pi/2 - atan(_u) when _minus is
// true, as high + low, for _u = _u_hi + _u_lo in [0, 1] and _quarters 0, 1
// or 2.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_atan_reduced(
    long double _u_hi,
    long double _u_lo,
    int         _quarters,
    bool        _minus
)
{
    // pi/2 = pio2_hi + pio2_lo, pio2_hi of 31 bits: exact as written
    const long double pio2_hi = 1.570796326734125614166259765625L;
    const long double pio2_lo = 6.077100506506192601475144209858469968755e-11L;
    // q pi/2, both parts exact multiples
    const long double base_hi = (long double)_quarters * pio2_hi;
    const long double base_lo = (long double)_quarters * pio2_lo;
    const long double sign    = _minus ? -1.0L : 1.0L;
    long double       c       = 0.0L;
    long double       at_hi   = 0.0L;
    long double       at_lo   = 0.0L;

    // c, the nearest of 0, 1/4, 1/2, 3/4 and 1, and atan c = at_hi + at_lo
    if (_u_hi >= 0.875L)
    {
        c     = 1.0L;
        at_hi = 3373259426.0L / 4294967296.0L;
        at_lo = 3.038550253253096300737572104929234984378e-11L;
    }
    else if (_u_hi >= 0.625L)
    {
        c     = 0.75L;
        at_hi = 2763816217.0L / 4294967296.0L;
        at_lo = 4.817137162794350606107263804151059111531e-11L;
    }
    else if (_u_hi >= 0.375L)
    {
        c     = 0.5L;
        at_hi = 3982702635.0L / 8589934592.0L;
        at_lo = 6.299816718559209976839402028537054286120e-12L;
    }
    else if (_u_hi >= 0.125L)
    {
        c     = 0.25L;
        at_hi = 4208701385.0L / 17179869184.0L;
        at_lo = 2.274351222528987867221331091414409838118e-11L;
    }

    {
        // t = (u - c) / (1 + c u): the numerator exact, the denominator as
        // a pair, the quotient with its residual
        const long double num  = _u_hi - c;
        const struct dj_internal_math_ld_pair cu =
            d_internal_math_ld_product_exact(c, _u_hi);
        const struct dj_internal_math_ld_pair den =
            d_internal_math_ld_two_sum(1.0L, cu.high);
        const long double den_lo = den.low + (cu.low + (c * _u_lo));
        const long double t_hi   = num / den.high;
        const struct dj_internal_math_ld_pair td =
            d_internal_math_ld_product_exact(t_hi, den.high);
        const long double t_lo   = ( ((num - td.high) - td.low) +
                                     (_u_lo - (t_hi * den_lo)) ) / den.high;
        const long double z      = t_hi * t_hi;
        long double       h      = 0.0L;
        int               j      = 0;

        // -1/3 + z/5 - z^2/7 + ..., from its 24th term in
        for (j = 24; j >= 1; --j)
        {
            h = ((((j % 2) != 0) ? -1.0L : 1.0L) /
                 (long double)((2 * j) + 1)) +
                (z * h);
        }

        {
            // atan t - t
            const long double tail = (t_hi * z) * h;
            // q pi/2 +- atan c is exact, and so is its sum with +-t; then
            // the small parts below them
            const struct dj_internal_math_ld_pair head =
                d_internal_math_ld_two_sum(base_hi + (sign * at_hi),
                                           sign * t_hi);
            const long double small = head.low +
                                      ( (base_lo + (sign * at_lo)) +
                                        (sign * (t_lo + tail)) );

            return d_internal_math_ld_two_sum(head.high, small);
        }
    }
}

// 1.6    Arc-sine and arc-cosine
//------------------------------------------------------------------------------
// asin x = atan(x / s) and acos x = atan(s / x), s = sqrt(1 - x^2), each with
// the smaller of |x| and s divided into the larger (section 1.5). s comes
// from (1 - |x|)(1 + |x|), each factor and their product kept as pairs, and
// its root's low part from the residual q - s^2, so that s is as precise as
// the rest; then the quotient is a pair too.

// d_internal_math_ld_cathetus
//   sqrt(1 - _a^2) as high + low, for 0 <= _a <= 1: the other leg of a right
// triangle whose hypotenuse is 1 and one leg _a.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_cathetus(
    long double _a
)
{
    // 1 - a and 1 + a, exactly
    const struct dj_internal_math_ld_pair below =
        d_internal_math_ld_two_sum(1.0L, -_a);
    const struct dj_internal_math_ld_pair above =
        d_internal_math_ld_two_sum(1.0L, _a);
    // their product, q = 1 - a^2, as q_hi + q_lo (fast two-sum)
    const struct dj_internal_math_ld_pair p =
        d_internal_math_ld_product_exact(below.high, above.high);
    const long double rest = p.low + ( (below.high * above.low) +
                                       (below.low * above.high) );
    const long double q_hi = p.high + rest;
    const long double q_lo = rest - (q_hi - p.high);
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // q is 0 only at a = 1; elsewhere its correctly rounded root, and the
    // residual's share below it
    if (q_hi > 0.0L)
    {
        const long double root = d_math_ld_sqrt_c(q_hi);
        const struct dj_internal_math_ld_pair square =
            d_internal_math_ld_product_exact(root, root);

        result.high = root;
        result.low  = (((q_hi - square.high) - square.low) + q_lo) /
                      (2.0L * root);
    }

    return result;
}

// d_internal_math_ld_asin_of
//   asin(_x) as high + low, for -1 <= _x <= 1.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_asin_of(
    long double _x
)
{
    const long double ax = (_x < 0.0L) ? -_x : _x;
    const struct dj_internal_math_ld_pair s = d_internal_math_ld_cathetus(ax);
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // up to 1/sqrt(2): atan(|x| / s); past it pi/2 - atan(s / |x|)
    if (ax <= s.high)
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(ax, 0.0L, s.high, s.low);

        result = d_internal_math_ld_atan_reduced(u.high, u.low, 0, false);
    }
    else
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(s.high, s.low, ax, 0.0L);

        result = d_internal_math_ld_atan_reduced(u.high, u.low, 1, true);
    }

    // asin(-x) = -asin x
    if (_x < 0.0L)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// d_internal_math_ld_acos_of
//   acos(_x) as high + low, for -1 <= _x <= 1.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_acos_of(
    long double _x
)
{
    const long double ax = (_x < 0.0L) ? -_x : _x;
    const struct dj_internal_math_ld_pair s = d_internal_math_ld_cathetus(ax);

    // from 1/sqrt(2) out: atan(s / |x|), and pi less it below 0
    if (s.high <= ax)
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(s.high, s.low, ax, 0.0L);

        return (_x > 0.0L) ? d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             0, false)
                           : d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             2, true);
    }

    // within it: pi/2 - atan(x / s), and pi/2 + atan(|x| / s) below 0
    {
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_ratio(ax, 0.0L, s.high, s.low);

        return (_x < 0.0L) ? d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             1, false)
                           : d_internal_math_ld_atan_reduced(u.high, u.low,
                                                             1, true);
    }
}
"""

# the pair each family's square root settles its last bit with; the long
# double family's is section 1's, which the kernels use too
ELEMENTARY_PAIR = r"""// dj_internal_math_@S@_pair
//   struct: a value as an unevaluated sum, high + low, the low part below the
// high part's rounding.
struct dj_internal_math_@S@_pair
{
    @T@ high;
    @T@ low;
};
"""

ELEMENTARY_PAIR_LD = r"""// dj_internal_math_ld_pair is section 1's (1.1), which the kernels share.
"""

ELEMENTARY_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

@PAIR@
// d_internal_math_@S@_nan
//   a quiet NaN: a constant expression where the build defines NAN.
D_MATH_FN @T@
d_internal_math_@S@_nan(void)
{
#if defined(NAN)
    return (@T@)NAN;
#else
    return (@T@)(HUGE_VAL - HUGE_VAL);
#endif
}

// d_internal_math_@S@_round_pair
//   a kernel's _hi + _lo rounded once to the family's type: _hi is the sum
// rounded to long double, and converting it rounds again, which is the same
// rounding but where _hi lies exactly between two values of the type - the
// first rounding having landed on the midpoint, as it does for log(1 - 2^-52)
// and exp(2^-53) - and then _lo's sign decides. A finite _hi past the
// type's range is an infinity, but for the midpoint above the largest finite
// value with the sum below it, which rounds there. A long double is _hi.
D_MATH_FN @T@
d_internal_math_@S@_round_pair(
    long double _hi,
    long double _lo
)
{
    const @T@ near = (@T@)_hi;
    // what the second rounding moved, exactly: at most half a gap
    const long double rest = _hi - (long double)near;

@OVERFLOW{@
    // a finite _hi the conversion took past the largest finite value
    if ( ( (near > @MAX@) ||
           (near < -@MAX@) ) &&
         (_hi <= LDBL_MAX) &&
         (_hi >= -LDBL_MAX) )
    {
        const long double gap = (long double)@MAX@ -
                                (long double)d_math_@S@_next_down(@MAX@);
        const long double mid = (near > (@T@)0) ?
                                    ((long double)@MAX@ + (gap / 2.0L)) :
                                    (-(long double)@MAX@ - (gap / 2.0L));

        // the midpoint above it, the sum below: the largest finite value
        if ( (_hi == mid) &&
             ( (near > (@T@)0) ? (_lo < 0.0L) : (_lo > 0.0L) ) )
        {
            return (near > (@T@)0) ? @MAX@ : -@MAX@;
        }

        return near;
    }
@}OVERFLOW@

    // _hi rounded down to near, from the midpoint, and the sum lies above it
    if ( (rest > 0.0L) &&
         (_lo > 0.0L) )
    {
        const @T@ up = d_math_@S@_next_up(near);

        if ((rest + rest) == ((long double)up - (long double)near))
        {
            return up;
        }
    }

    // _hi rounded up to near, from the midpoint, and the sum lies below it
    if ( (rest < 0.0L) &&
         (_lo < 0.0L) )
    {
        const @T@ down = d_math_@S@_next_down(near);

        if ((rest + rest) == ((long double)down - (long double)near))
        {
            return down;
        }
    }

    return near;
}

// d_internal_math_@S@_abs
//   |_x| by comparison, as the C++ headers took it (a -0 stays -0); a
// constant expression, where fabs is not.
D_MATH_FN @T@
d_internal_math_@S@_abs(
    @T@ _x
)
{
    return (_x < (@T@)0) ? -_x : _x;
}

// d_internal_math_@S@_square_exact
//   _c * _c exactly, as high + low: the low part by a fused multiply-add
// where the target has one for @NAME@, and otherwise by Dekker's product,
// the factors split by Veltkamp's constant 2^ceil(P/2) + 1 (float_order.h);
// both exact, so both give the same bits (math_common.h, section 5). _c must
// be normal and its square finite.
D_MATH_FN struct dj_internal_math_@S@_pair
d_internal_math_@S@_square_exact(
    @T@ _c
)
{
    struct dj_internal_math_@S@_pair result = { (@T@)0, (@T@)0 };

#if D_MATH_@PFX@_FMA
    if (D_MATH_RUN_TIME())
    {
        result.high = _c * _c;
        result.low  = D_MATH_@PFX@_FMA_OF(_c, _c, -result.high);

        return result;
    }
#endif  // D_MATH_@PFX@_FMA

    const @T@ split = d_internal_math_@S@_veltkamp();
    const @T@ t     = split * _c;
    const @T@ hi    = t - (t - _c);
    const @T@ lo    = _c - hi;

    result.high = _c * _c;
    result.low  = (((hi * hi) - result.high) + (((@T@)2 * hi) * lo)) +
                  (lo * lo);

    return result;
}

// dj_internal_math_@S@_wide
//   struct: an unsigned integer of two dj_math_umax limbs, high * 2^bits + low.
struct dj_internal_math_@S@_wide
{
    dj_math_umax high;
    dj_math_umax low;
};

// d_internal_math_@S@_limb_bits
//   the bits of dj_math_umax.
D_MATH_FN int
d_internal_math_@S@_limb_bits(void)
{
    return (int)(sizeof(dj_math_umax) * 8u);
}

// d_internal_math_@S@_wide_shift
//   _value * 2^_shift as two limbs, for 0 < _shift < the limb's bits.
D_MATH_FN struct dj_internal_math_@S@_wide
d_internal_math_@S@_wide_shift(
    dj_math_umax _value,
    int          _shift
)
{
    const int bits = d_internal_math_@S@_limb_bits();
    struct dj_internal_math_@S@_wide result = { 0u, 0u };

    result.high = _value >> ((bits - _shift) % bits);
    result.low  = _value << (_shift % bits);

    return result;
}

// d_internal_math_@S@_wide_square
//   _a * _a as two limbs: _a split into half-limbs, so each partial product
// fits one limb, and the cross term added twice with its carries.
D_MATH_FN struct dj_internal_math_@S@_wide
d_internal_math_@S@_wide_square(
    dj_math_umax _a
)
{
    const int          half  = d_internal_math_@S@_limb_bits() / 2;
    const dj_math_umax mask  = (((dj_math_umax)1) << half) - 1u;
    const dj_math_umax ah    = _a >> half;
    const dj_math_umax al    = _a & mask;
    const dj_math_umax cross = ah * al;
    const dj_math_umax lo_in = cross << half;
    const dj_math_umax hi_in = cross >> half;
    struct dj_internal_math_@S@_wide result = { 0u, 0u };
    int pass = 0;

    result.high = ah * ah;
    result.low  = al * al;

    // the cross term twice, each time with the carry out of the low limb
    for (pass = 0; pass < 2; ++pass)
    {
        result.low  += lo_in;
        result.high += hi_in + ((result.low < lo_in) ? 1u : 0u);
    }

    return result;
}

// d_internal_math_@S@_wide_less
//   whether _a < _b.
D_MATH_FN bool
d_internal_math_@S@_wide_less(
    struct dj_internal_math_@S@_wide _a,
    struct dj_internal_math_@S@_wide _b
)
{
    return ( (_a.high < _b.high) ||
             ( (_a.high == _b.high) &&
               (_a.low < _b.low) ) );
}

// d_internal_math_@S@_sqrt_settle
//   the last bit of the root of _m in [1, 4), settled in integers: with S =
// 2^(P-1), the root is R / S for an integer R, the midpoint above it is
// (2R + 1) / 2S, and sqrt(m) passes it exactly when 4 S M > (2R + 1)^2, M =
// m S being an integer too. Integer arithmetic is the same under every
// floating-point evaluation method - an x87's excess precision, which
// breaks Dekker's product, included - and in constant evaluation.
D_MATH_FN @T@
d_internal_math_@S@_sqrt_settle(
    @T@ _m,
    @T@ _root
)
{
    const @T@          scale = d_internal_math_@S@_pow2(@P@ - 1);
    const dj_math_umax s     = (dj_math_umax)scale;
    const dj_math_umax m     = (dj_math_umax)(_m * scale);
    const struct dj_internal_math_@S@_wide four_s_m =
        d_internal_math_@S@_wide_shift(m, @P@ + 1);
    dj_math_umax      r     = (_root >= (@T@)2) ? ((2u * s) - 1u)
                                                : (dj_math_umax)(_root * scale);
    int               tries = 0;

    // up while the root lies past the midpoint above...
    for (tries = 0;
         ( (tries < 4) &&
           (d_internal_math_@S@_wide_less(
                d_internal_math_@S@_wide_square((2u * r) + 1u),
                four_s_m)) );
         ++tries)
    {
        ++r;
    }

    // ...down while it lies short of the midpoint below (from 1 it cannot)
    for (tries = 0;
         ( (tries < 4) &&
           (r > s) &&
           (d_internal_math_@S@_wide_less(
                four_s_m,
                d_internal_math_@S@_wide_square((2u * r) - 1u))) );
         ++tries)
    {
        --r;
    }

    return ((@T@)r / scale);
}

// dj_internal_math_@S@_digits
//   struct: a whole number in digits of 16 bits, the least first, each held
// in a dj_math_umax: a digit times a digit, with a digit and a carry added,
// stays below 2^32, which the narrowest dj_math_umax holds. Room for numbers
// below 2^(2P+4): sqrt_settle_digits's, where two limbs do not hold them -
// S M, the largest, lies below 2^(2P).
struct dj_internal_math_@S@_digits
{
    dj_math_umax digit[2 * ((@P@ + 17) / 16)];
};

// d_internal_math_@S@_digit_count
//   the digits a dj_internal_math_@S@_digits holds.
D_MATH_FN int
d_internal_math_@S@_digit_count(void)
{
    return (2 * ((@P@ + 17) / 16));
}

// d_internal_math_@S@_digits_of
//   the whole part of _value, from 0 to below 2^64, in digits: its two
// 32-bit halves first, each a whole number that converts exactly, whatever
// precision the build evaluates the family in.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_of(
    @T@ _value
)
{
    const @DG_T@ word = (@T@)4294967296.0;  // 2^32
    const @DG_UMAX@ high = (dj_math_umax)(_value / word);
    const @DG_UMAX@ low  =
        (dj_math_umax)(_value - ((@T@)high * word));
    struct dj_internal_math_@S@_digits result = { { 0u } };

    result.digit[0] = low & 0xFFFFu;
    result.digit[1] = (low >> 16) & 0xFFFFu;
    result.digit[2] = high & 0xFFFFu;
    result.digit[3] = (high >> 16) & 0xFFFFu;

    return result;
}

// d_internal_math_@S@_digits_value
//   _a as a value of the family, from the most significant digit down:
// exact while _a lies below 2^P, as every partial sum does then.
D_MATH_FN @T@
d_internal_math_@S@_digits_value(
    struct dj_internal_math_@S@_digits _a
)
{
    @T@ result = (@T@)0;
    @DG_INT@ k      = 0;

    for (k = d_internal_math_@S@_digit_count() - 1; k >= 0; --k)
    {
        result = (result * (@T@)65536) + (@T@)_a.digit[k];
    }

    return result;
}

// d_internal_math_@S@_digits_step
//   _a + 1 where _up, and else _a - 1, _a above 0: a carry or a borrow taken
// up the digits.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_step(
    struct dj_internal_math_@S@_digits _a,
    @DG_BOOL@ _up
)
{
    struct dj_internal_math_@S@_digits result = _a;
    int k = 0;

    // a digit that can take the step takes it, and ends the carry or the
    // borrow; one that cannot wraps, and passes it on
    for (k = 0; k < d_internal_math_@S@_digit_count(); ++k)
    {
        if (_up)
        {
            if (result.digit[k] < 0xFFFFu)
            {
                ++result.digit[k];

                break;
            }

            result.digit[k] = 0u;
        }
        else
        {
            if (result.digit[k] > 0u)
            {
                --result.digit[k];

                break;
            }

            result.digit[k] = 0xFFFFu;
        }
    }

    return result;
}

// d_internal_math_@S@_digits_shift
//   _a * 2^_bits: each digit moved up _bits / 16 places and _bits % 16 bits,
// what passes 16 bits carried into the next. The product must fit.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_shift(
    struct dj_internal_math_@S@_digits _a,
    @DG_INTP@ _bits
)
{
    const int    whole = _bits / 16;
    const int    part  = _bits % 16;
    dj_math_umax carry = 0u;
    int          k     = 0;
    struct dj_internal_math_@S@_digits result = { { 0u } };

    for (k = 0; (k + whole) < d_internal_math_@S@_digit_count(); ++k)
    {
        const dj_math_umax moved = (_a.digit[k] << part) | carry;

        result.digit[k + whole] = moved & 0xFFFFu;
        carry                   = moved >> 16;
    }

    return result;
}

// d_internal_math_@S@_low_digit_count
//   the digits, the lowest, that sqrt_settle_digits takes its residual in:
// P + 9 bits or more, so that the residual, below 2^(P+4) in size, keeps its
// sign in their top bit.
D_MATH_FN int
d_internal_math_@S@_low_digit_count(void)
{
    return ((@P@ + 24) / 16);
}

// d_internal_math_@S@_digits_low_add
//   _a + _b where _up, and else _a - _b, in the low digits alone, modulo
// 2^16 to their count: a digit at a time, the carry or the borrow taken into
// the next, and the one out of the top dropped. The digits above are 0.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_low_add(
    struct dj_internal_math_@S@_digits _a,
    struct dj_internal_math_@S@_digits _b,
    @DG_BOOL@ _up
)
{
    dj_math_umax carry = 0u;
    int          k     = 0;
    struct dj_internal_math_@S@_digits result = { { 0u } };

    for (k = 0; k < d_internal_math_@S@_low_digit_count(); ++k)
    {
        if (_up)
        {
            const dj_math_umax sum = (_a.digit[k] + _b.digit[k]) + carry;

            result.digit[k] = sum & 0xFFFFu;
            carry           = sum >> 16;
        }
        else
        {
            // the digit above lends 2^16, and the borrow pays it back
            const dj_math_umax difference =
                ((_a.digit[k] + 0x10000u) - _b.digit[k]) - carry;

            result.digit[k] = difference & 0xFFFFu;
            carry           = 1u - (difference >> 16);
        }
    }

    return result;
}

// d_internal_math_@S@_digits_low_square
//   _a * _a in the low digits alone, modulo 2^16 to their count: only the
// products of two digits that land there, each added in with the digit
// already there and the carry, which stays below 2^32; the carry out of the
// top dropped.
D_MATH_FN struct dj_internal_math_@S@_digits
d_internal_math_@S@_digits_low_square(
    struct dj_internal_math_@S@_digits _a
)
{
    const int low = d_internal_math_@S@_low_digit_count();
    int       i   = 0;
    int       j   = 0;
    struct dj_internal_math_@S@_digits result = { { 0u } };

    for (i = 0; i < low; ++i)
    {
        dj_math_umax carry = 0u;

        for (j = 0; (i + j) < low; ++j)
        {
            const dj_math_umax sum = result.digit[i + j] +
                                     (_a.digit[i] * _a.digit[j]) + carry;

            result.digit[i + j] = sum & 0xFFFFu;
            carry               = sum >> 16;
        }
    }

    return result;
}

// d_internal_math_@S@_digits_negative
//   whether the low digits, read in two's complement, are below 0: the top
// bit of the top one.
D_MATH_FN bool
d_internal_math_@S@_digits_negative(
    struct dj_internal_math_@S@_digits _a
)
{
    return ((_a.digit[d_internal_math_@S@_low_digit_count() - 1] >> 15) != 0u);
}

// d_internal_math_@S@_digits_less
//   whether _a < _b: the most significant digit that differs decides.
D_MATH_FN bool
d_internal_math_@S@_digits_less(
    struct dj_internal_math_@S@_digits _a,
    struct dj_internal_math_@S@_digits _b
)
{
    int k = 0;

    for (k = d_internal_math_@S@_digit_count() - 1; k >= 0; --k)
    {
        if (_a.digit[k] != _b.digit[k])
        {
            return (_a.digit[k] < _b.digit[k]);
        }
    }

    return false;
}

// d_internal_math_@S@_sqrt_settle_digits
//   sqrt_settle where two limbs do not hold (2^(P+1))^2 - a double where
// dj_math_umax has 32 bits - in digits of 16 bits instead, exact in
// integers, so that nothing depends on how the build evaluates floating
// point. R / S is the rounded root when (2R - 1)^2 < 4 S M < (2R + 1)^2,
// that is when the residual G = S M - R^2 + R - 1 lies in [0, 2R): R is
// short where G - 2R, G once R steps up, is 0 or more, and past where G is
// below 0; a step down adds 2R, R the new one. While R lies within four
// steps of the root's, G is below 2^(P+4) in size, so it is taken in the low
// digits alone, whose top bit keeps its sign: R is squared once, and only as
// far as they reach. R starts from the root's whole part, which a root held
// in a wider register than the family's leaves within a step or two of it.
// For a precision P up to 63, so that M and R lie below 2^64.
D_MATH_FN @T@
d_internal_math_@S@_sqrt_settle_digits(
    @T@ _m,
    @T@ _root
)
{
    const @T@ scale = d_internal_math_@S@_pow2(@P@ - 1);
    const struct dj_internal_math_@S@_digits s =
        d_internal_math_@S@_digits_of(scale);
    // R from the root's whole part in units of 1 / S, below 2S
    const @T@ top = ((@T@)2 * scale) - (@T@)1;
    struct dj_internal_math_@S@_digits r =
        d_internal_math_@S@_digits_of((_root >= (@T@)2) ? top
                                                        : (_root * scale));
    // G = S M - R^2 + (R - 1), M = m S
    struct dj_internal_math_@S@_digits g =
        d_internal_math_@S@_digits_low_add(
            d_internal_math_@S@_digits_low_add(
                d_internal_math_@S@_digits_shift(
                    d_internal_math_@S@_digits_of(_m * scale),
                    @P@ - 1),
                d_internal_math_@S@_digits_low_square(r),
                false),
            d_internal_math_@S@_digits_step(r, false),
            true);
    // G - 2R
    struct dj_internal_math_@S@_digits next =
        d_internal_math_@S@_digits_low_add(
            g,
            d_internal_math_@S@_digits_low_add(r, r, true),
            false);
    int tries = 0;

    // up while the root lies past the midpoint above...
    for (tries = 0;
         ( (tries < 4) &&
           (!d_internal_math_@S@_digits_negative(next)) );
         ++tries)
    {
        g    = next;
        r    = d_internal_math_@S@_digits_step(r, true);
        next = d_internal_math_@S@_digits_low_add(
                   g,
                   d_internal_math_@S@_digits_low_add(r, r, true),
                   false);
    }

    // ...down while it lies short of the midpoint below (from 1 it cannot)
    for (tries = 0;
         ( (tries < 4) &&
           (d_internal_math_@S@_digits_negative(g)) &&
           (d_internal_math_@S@_digits_less(s, r)) );
         ++tries)
    {
        r = d_internal_math_@S@_digits_step(r, false);
        g = d_internal_math_@S@_digits_low_add(
                g,
                d_internal_math_@S@_digits_low_add(r, r, true),
                true);
    }

    return (d_internal_math_@S@_digits_value(r) / scale);
}

// d_internal_math_@S@_sqrt_unit
//   the correctly rounded square root of _m in [1, 4): Newton's iteration
// from a line within 6% of it - six steps pass any precision to a quad's -
// then the last bit settled. Where two limbs hold (2^(P+1))^2 - float, and
// double with a 64-bit dj_math_umax - it is settled in integers
// (sqrt_settle), exact however the build evaluates floating point. Where
// they do not and P is below 64 - a double where dj_math_umax has 32 bits,
// which an x87 holds in registers wider than a double - in integers too, in
// digits of 16 bits (sqrt_settle_digits). Otherwise - a long double of 64
// bits or more - by the residual m - root^2, taken exactly by Dekker's
// product, against the midpoints with the root's neighbours: arithmetic
// that is its own evaluation format, so that no excess precision intervenes.
D_MATH_FN @T@
d_internal_math_@S@_sqrt_unit(
    @T@ _m
)
{
    @T@ root  = (@T@)0.6 + ((@T@)0.4 * _m);
    int i     = 0;
    int tries = 0;

    // Newton's iteration
    for (i = 0; i < 6; ++i)
    {
        root = (@T@)0.5 * (root + (_m / root));
    }

    // the last bit, in integers where they reach
    if (((2 * @P@) + 2) <= (2 * d_internal_math_@S@_limb_bits()))
    {
        return d_internal_math_@S@_sqrt_settle(_m, root);
    }

    // in digits where they do not, for a precision below 64: Dekker's
    // product, below, might be evaluated wider than the family - on the
    // x87, by clang even where long double is a double
    if (@P@ < 64)
    {
        return d_internal_math_@S@_sqrt_settle_digits(_m, root);
    }

    // the last bit: up past the upper midpoint, down past the lower one
    for (tries = 0; tries < 3; ++tries)
    {
        const struct dj_internal_math_@S@_pair square =
            d_internal_math_@S@_square_exact(root);
        const @T@ residual = (_m - square.high) - square.low;
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(root);
        const @T@ above = b.power * @EPS@;
        const @T@ below = (root == b.power) ? (above / (@T@)2) : above;

        if ((residual - (root * above)) > ((above * above) / (@T@)4))
        {
            root = d_math_@S@_next_up(root);
        }
        else if ((residual + (root * below)) < ((below * below) / (@T@)4))
        {
            root = d_math_@S@_next_down(root);
        }
        else
        {
            break;
        }
    }

    return root;
}

// @SECTION@.2    Square root
//------------------------------------------------------------------------------
// sqrt_c is the correctly rounded square root in pure arithmetic, so the same
// text is a constant expression from C++14, and it agrees with sqrt, the C
// library's (correctly rounded too, by IEEE 754), bit for bit: a face may use
// either and not know which ran. Where the target evaluates the family wider
// than its type - float and double on a 32-bit x87 - the library's root can
// come back wider too (glibc's sqrtf leaves the x87's 64 bits), and sqrt
// stores it, so that it is the family's, as sqrt_c's is. -0 stays -0, a
// negative or NaN gives NaN, +inf stays +inf.

D_MATH_FN @T@
d_math_@S@_sqrt_c(
    @T@ _x
)
{
    // NaN, zero (of either sign) and +inf are their own roots
    if ( (_x != _x) ||
         (_x == (@T@)0) ||
         (_x > @MAX@) )
    {
        return _x;
    }

    // a negative value has none
    if (_x < (@T@)0)
    {
        return d_internal_math_@S@_nan();
    }

    {
        // a subnormal is first scaled into the normal range by an even power
        const int lift  = (_x < @MIN@) ? (((@P@ + 1) / 2) * 2) : 0;
        const @T@ value = (lift > 0) ? (_x * d_internal_math_@S@_pow2(lift))
                                     : _x;
        const struct dj_internal_math_@S@_binade b =
            d_internal_math_@S@_binade_of(value);
        // value = m * 4^k with m in [1, 4); k = floor(E / 2)
        const int k = (b.exponent >= 0) ? (b.exponent / 2)
                                        : -((1 - b.exponent) / 2);
        const @T@ m = value / d_internal_math_@S@_pow2(2 * k);

        return ( d_internal_math_@S@_sqrt_unit(m) *
                 d_internal_math_@S@_pow2(k - (lift / 2)) );
    }
}

D_MATH_FN_RT @T@
d_math_@S@_sqrt(
    @T@ _x
)
{
#ifdef __cplusplus
    const @T@ root = std::sqrt(_x);
#else
    const @T@ root = @CSQRT@(_x);
#endif  // __cplusplus

    // where the target evaluates the family wider than its type, stored, so
    // that it is a @NAME@: a compiler takes a call's result to be of its
    // type, and keeps what the library left in the register
    if (@EVAL_WIDE@)
    {
        volatile @T@ stored = root;

        return stored;
    }

    return root;
}

// @SECTION@.3    Arc-sine and arc-cosine
//------------------------------------------------------------------------------
// asin_c and acos_c are pure arithmetic, constant expressions from C++14: the
// kernels' pairs (section 1.6) rounded once to the family - the correctly
// rounded value but where the exact one lies within the kernels' error of a
// midpoint, and so within an ulp of asin and acos, which are the C library's.
// A value outside [-1, 1], or NaN, gives NaN, as the library's do; asin keeps
// a zero's sign, and acos(1) is +0.

D_MATH_FN @T@
d_math_@S@_asin_c(
    @T@ _x
)
{
    // NaN, and zero of either sign, are their own arc-sines
    if ( (_x != _x) ||
         (_x == (@T@)0) )
    {
        return _x;
    }

    // outside the domain
    if (!( (_x >= (@T@)-1) &&
           (_x <= (@T@)1) ))
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_asin_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_asin(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::asin(_x);
#else
    return @CASIN@(_x);
#endif
}

D_MATH_FN @T@
d_math_@S@_acos_c(
    @T@ _x
)
{
    // outside the domain, and NaN
    if (!( (_x >= (@T@)-1) &&
           (_x <= (@T@)1) ))
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_acos_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_acos(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::acos(_x);
#else
    return @CACOS@(_x);
#endif
}

// @SECTION@.4    Sine and cosine
//------------------------------------------------------------------------------
// sin_c and cos_c are pure arithmetic, constant expressions from C++14, at
// every finite argument: the kernel's pair (section 1.4) - the argument
// reduced exactly, however large - rounded once to the family: the correctly
// rounded value but where the exact one lies within the kernel's error of a
// midpoint, and so within an ulp of sin and cos, which are the C library's.
// NaN and an infinity give NaN; -0 stays -0 for sin.

D_MATH_FN @T@
d_math_@S@_sin_c(
    @T@ _x
)
{
    // NaN and the infinities
    if (!( (_x <= @MAX@) &&
           (_x >= -@MAX@) ))
    {
        return d_internal_math_@S@_nan();
    }

    // zero, of either sign, is its own sine
    if (_x == (@T@)0)
    {
        return _x;
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_trig((long double)_x, false);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN @T@
d_math_@S@_cos_c(
    @T@ _x
)
{
    // NaN and the infinities
    if (!( (_x <= @MAX@) &&
           (_x >= -@MAX@) ))
    {
        return d_internal_math_@S@_nan();
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_trig((long double)_x, true);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_sin(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::sin(_x);
#else
    return @CSIN@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_cos(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::cos(_x);
#else
    return @CCOS@(_x);
#endif
}
"""

ELEMENTARY_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS (long double)
    -------------------------
    1.  Exact sums and products
    2.  The digits of 2/pi
    3.  Reduction by pi/2
    4.  Sine and cosine of the reduced argument
    5.  The arc-tangent of the unit interval
    6.  Arc-sine and arc-cosine
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Square root
    3.  Arc-sine and arc-cosine
    4.  Sine and cosine
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Square root
    3.  Arc-sine and arc-cosine
    4.  Sine and cosine
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Square root
    3.  Arc-sine and arc-cosine
    4.  Sine and cosine
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_ELEMENTARY_H
#define DJINTERP_C_MATH_ELEMENTARY_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::sqrt, std::asin, std::acos, std::sin, ...
#endif  // __cplusplus
#include <float.h>           // LDBL_MANT_DIG, LDBL_MAX_EXP, LDBL_EPSILON
#include <math.h>            // sqrtf ... cosl, HUGE_VAL, NAN
// djinterp
#include "./float_order.h"   // d_math_*_next_up, d_internal_math_*_binade_of
#include "./math_common.h"   // D_MATH_FN


// the arc-sine's and arc-cosine's kernel (section 1.6) takes its root in
// long double, the long double family's (section 4.2)
D_MATH_FN long double
d_math_ld_sqrt_c(
    long double _x
);
"""

ELEMENTARY_TAIL = r"""

#endif  // DJINTERP_C_MATH_ELEMENTARY_H
"""

ELEMENTARY_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_sqrt_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_sqrt", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_asin_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_asin", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_acos_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_acos", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_sin_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_sin", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_cos_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_cos", [("@T@", "_x")]),
]


def elementary():
    lines = [
        "Elementary functions: the C core's square root, arc-sine, arc-cosine,",
        "sine and cosine, for float, double and long double, and the long double",
        "kernels its other floating-point headers share.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice. The _c form is pure arithmetic, so the same",
        "text is a constant expression from C++14; the plain form calls the C",
        "library (std:: in C++, which has the float and long double overloads from",
        "C++98), fast at run time. sqrt_c is correctly rounded, so the two square",
        "roots agree bit for bit. asin_c, acos_c, sin_c and cos_c are computed",
        "once, in long double, as exact pairs - the sine's argument reduced by",
        "pi/2 exactly, however large - and rounded once to the family, within an",
        "ulp of the library's. A face picks one by level - from C++20 by",
        "std::is_constant_evaluated - and, for the square root, cannot be seen to",
        "have picked.",
    ]
    text = banner("elementary.h", lines, DATE, "2026.11.24")
    text += "\n" + ELEMENTARY_HEAD
    text += ELEMENTARY_KERNELS.replace("@TWO_OVER_PI@", two_over_pi_literal()) \
                              .replace("@DIGITS@", str(TWO_OVER_PI_DIGITS)) \
                              .replace("@BITS@", "{:,}".format(24 * TWO_OVER_PI_DIGITS))
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = ELEMENTARY_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        body = body.replace("@PAIR@", ELEMENTARY_PAIR_LD if suffix == "ld" else ELEMENTARY_PAIR)
        # round_pair's overflow: a long double is _hi itself, never past its
        # range where _hi is not, so its family has no such branch
        if suffix == "ld":
            body = re.sub(r"@OVERFLOW\{@\n.*?@\}OVERFLOW@\n\n?", "", body,
                          flags=re.S)
        else:
            body = body.replace("@OVERFLOW{@\n", "").replace("@}OVERFLOW@\n", "")
        body = body.replace("@CSQRT@", {"float": "sqrtf", "double": "sqrt", "long double": "sqrtl"}[ctype])
        body = body.replace("@CASIN@", {"float": "asinf", "double": "asin", "long double": "asinl"}[ctype])
        body = body.replace("@CACOS@", {"float": "acosf", "double": "acos", "long double": "acosl"}[ctype])
        body = body.replace("@CSIN@", {"float": "sinf", "double": "sin", "long double": "sinl"}[ctype])
        body = body.replace("@CCOS@", {"float": "cosf", "double": "cos", "long double": "cosl"}[ctype])
        # math_common.h's word on whether the target evaluates the family
        # wider than its type
        body = body.replace("@EVAL_WIDE@", "D_MATH_%s_EVAL_WIDE" % prefix)
        # the digits' declarations and parameters in line, whatever the
        # family's type and suffix
        width = max(len(ctype), len("dj_math_umax"))
        digits = "struct dj_internal_math_%s_digits" % suffix
        body = body.replace("@DG_T@", ctype.ljust(width)).replace("@DG_UMAX@", "dj_math_umax".ljust(width))
        body = body.replace("@DG_INT@", "int".ljust(len(ctype)))
        body = body.replace("@DG_BOOL@", "bool".ljust(len(digits))).replace("@DG_INTP@", "int".ljust(len(digits)))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", ELEMENTARY_OVERLOADS)
    text += ELEMENTARY_TAIL
    return text
