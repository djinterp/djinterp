"""gen_c_math.dd - the text of dd.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (banner)
from .self_test import (DD_BOUNDS)


# -----------------------------------------------------------------------------
# dd.h: pairs of doubles (chunk 51, S417)
# -----------------------------------------------------------------------------

DD_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE PAIR
    --------
    1.  The type
    2.  Making and reading a pair
2.  EXACT SUMS AND PRODUCTS
    -----------------------
    1.  Sums
    2.  The product
3.  ARITHMETIC ON PAIRS
    -------------------
    1.  Sums and differences
    2.  Products
    3.  Quotients
    4.  The square root
*/

#ifndef DJINTERP_C_MATH_DD_H
#define DJINTERP_C_MATH_DD_H 1

// djinterp
#include "./elementary.h"    // d_math_d_sqrt_c
#include "./float_order.h"   // d_internal_math_d_veltkamp
#include "./math_common.h"   // D_MATH_FN, D_MATH_DBL_FMA
"""

DD_BODY = r"""

//==============================================================================
// 1.  THE PAIR
//==============================================================================
// A pair holds high + low, unevaluated, |low| at most half an ulp of high:
// some 106 significant bits, twice double's. Every function here takes and
// gives such a normalized pair, as a correctly rounded high leaves it; a pair
// made by hand is normalized by d_math_dd_fast_two_sum.
//   No error-free transform here takes a product that is not exact, so that
// a compiler fusing a multiply and an add cannot break one: it would put the
// exact product in place of its rounding at each use, and the transform's
// low part would no longer be its error (math_common.h, section 5).


// 1.1    The type
//------------------------------------------------------------------------------

// dj_math_dd
//   struct: a pair of doubles, high + low, |low| <= ulp(high) / 2.
struct dj_math_dd
{
    double high;  // the value, rounded to double
    double low;   // the rest
};


// 1.2    Making and reading a pair
//------------------------------------------------------------------------------

// d_math_dd_of
//   _x as a pair, { _x, 0 }.
D_MATH_FN struct dj_math_dd
d_math_dd_of(
    double _x
)
{
    struct dj_math_dd result = { 0.0, 0.0 };

    result.high = _x;

    return result;
}

// d_math_dd_value
//   _x rounded to double: high + low, which is high for a normalized pair but
// where low is exactly half an ulp.
D_MATH_FN double
d_math_dd_value(
    struct dj_math_dd _x
)
{
    return _x.high + _x.low;
}

// d_math_dd_neg
//   -_x, exactly.
D_MATH_FN struct dj_math_dd
d_math_dd_neg(
    struct dj_math_dd _x
)
{
    struct dj_math_dd result = { 0.0, 0.0 };

    result.high = -_x.high;
    result.low  = -_x.low;

    return result;
}


//==============================================================================
// 2.  EXACT SUMS AND PRODUCTS
//==============================================================================


// 2.1    Sums
//------------------------------------------------------------------------------

// d_math_dd_two_sum
//   _a + _b exactly, as high + low, high its rounding: Knuth's 2Sum, six
// operations, for any doubles whose sum is finite.
D_MATH_FN struct dj_math_dd
d_math_dd_two_sum(
    double _a,
    double _b
)
{
    struct dj_math_dd result = { 0.0, 0.0 };
    const double      high   = _a + _b;
    const double      a_part = high - _b;
    const double      b_part = high - a_part;

    D_MATH_DBL_REFUSE(_a)

    result.high = high;
    result.low  = (_a - a_part) + (_b - b_part);

    return result;
}

// d_math_dd_fast_two_sum
//   _a + _b exactly, as high + low, high its rounding: Dekker's Fast2Sum,
// three operations, exact where _a is 0 or its exponent is at least _b's, as
// it is where |_a| >= |_b|.
D_MATH_FN struct dj_math_dd
d_math_dd_fast_two_sum(
    double _a,
    double _b
)
{
    struct dj_math_dd result = { 0.0, 0.0 };
    const double      high   = _a + _b;

    D_MATH_DBL_REFUSE(_a)

    result.high = high;
    result.low  = _b - (high - _a);

    return result;
}


// 2.2    The product
//------------------------------------------------------------------------------

// d_math_dd_two_product
//   _a * _b exactly, as high + low, high its rounding: the low part by a
// fused multiply-add where the target has one, and otherwise by Dekker's
// product, each factor split by Veltkamp's constant 2^27 + 1; both exact, so
// both give the same bits (math_common.h, section 5). Exact where the product
// is 0 or of magnitude 2^-969 to 2^1023, and each factor below 2^995.
D_MATH_FN struct dj_math_dd
d_math_dd_two_product(
    double _a,
    double _b
)
{
    struct dj_math_dd result = { 0.0, 0.0 };

    D_MATH_DBL_REFUSE(_a)

#if D_MATH_DBL_FMA
    if (D_MATH_RUN_TIME())
    {
        result.high = _a * _b;
        result.low  = D_MATH_DBL_FMA_OF(_a, _b, -result.high);

        return result;
    }
#endif  // D_MATH_DBL_FMA

    const double split = d_internal_math_d_veltkamp();
    const double ta    = split * _a;
    const double a_hi  = ta - (ta - _a);
    const double a_lo  = _a - a_hi;
    const double tb    = split * _b;
    const double b_hi  = tb - (tb - _b);
    const double b_lo  = _b - b_hi;
    const double high  = _a * _b;

    result.high = high;
    result.low  = ( (((a_hi * b_hi) - high) + (a_hi * b_lo)) +
                    (a_lo * b_hi) ) +
                  (a_lo * b_lo);

    return result;
}


//==============================================================================
// 3.  ARITHMETIC ON PAIRS
//==============================================================================
// Each result is within the bound stated of the exact one, relatively, in
// units of u^2 = 2^-106, where no operation overflows or underflows, and
// is a normalized pair. The sums and products are Joldes, Muller and
// Popescu's (ACM TOMS 44(2), 2017), with their proven bounds; where a
// compiler fuses a product into the add that follows, each becomes the
// paper's fused variant, whose bound is no larger. The quotients and the
// square root are written so that no inexact product reaches an error-free
// transform, and their bounds are proven with Gappa (chunk 51's record,
// proofs/), fused or not.


// 3.1    Sums and differences
//------------------------------------------------------------------------------

// d_math_dd_add_d
//   _x + _y: DWPlusFP, within 2u^2 / (1 - 2u) (< 2u^2 + 5u^3).
D_MATH_FN struct dj_math_dd
d_math_dd_add_d(
    struct dj_math_dd _x,
    double            _y
)
{
    const struct dj_math_dd s = d_math_dd_two_sum(_x.high, _y);

    return d_math_dd_fast_two_sum(s.high, _x.low + s.low);
}

// d_math_dd_add
//   _x + _y: AccurateDWPlusDW, within 3u^2 / (1 - 4u) (< 3u^2 + 13u^3).
D_MATH_FN struct dj_math_dd
d_math_dd_add(
    struct dj_math_dd _x,
    struct dj_math_dd _y
)
{
    const struct dj_math_dd s = d_math_dd_two_sum(_x.high, _y.high);
    const struct dj_math_dd t = d_math_dd_two_sum(_x.low, _y.low);
    const struct dj_math_dd v = d_math_dd_fast_two_sum(s.high, s.low + t.high);

    return d_math_dd_fast_two_sum(v.high, t.low + v.low);
}

// d_math_dd_sub_d
//   _x - _y, as d_math_dd_add_d of -_y.
D_MATH_FN struct dj_math_dd
d_math_dd_sub_d(
    struct dj_math_dd _x,
    double            _y
)
{
    return d_math_dd_add_d(_x, -_y);
}

// d_math_dd_sub
//   _x - _y, as d_math_dd_add of -_y.
D_MATH_FN struct dj_math_dd
d_math_dd_sub(
    struct dj_math_dd _x,
    struct dj_math_dd _y
)
{
    return d_math_dd_add(_x, d_math_dd_neg(_y));
}


// 3.2    Products
//------------------------------------------------------------------------------

// d_math_dd_mul_d
//   _x * _y: DWTimesFP2, within 3u^2; fused, DWTimesFP3, within 2u^2.
D_MATH_FN struct dj_math_dd
d_math_dd_mul_d(
    struct dj_math_dd _x,
    double            _y
)
{
    const struct dj_math_dd c   = d_math_dd_two_product(_x.high, _y);
    const double            cl2 = _x.low * _y;

    return d_math_dd_fast_two_sum(c.high, c.low + cl2);
}

// d_math_dd_mul
//   _x * _y: DWTimesDW1, within 7u^2; fused, DWTimesDW2, within 5u^2.
D_MATH_FN struct dj_math_dd
d_math_dd_mul(
    struct dj_math_dd _x,
    struct dj_math_dd _y
)
{
    const struct dj_math_dd c   = d_math_dd_two_product(_x.high, _y.high);
    const double            tl1 = _x.high * _y.low;
    const double            tl2 = _x.low * _y.high;

    return d_math_dd_fast_two_sum(c.high, c.low + (tl1 + tl2));
}


// 3.3    Quotients
//------------------------------------------------------------------------------

// d_math_dd_div_d
//   _x / _y: Joldes, Muller and Popescu's DWDivFP, th = xh / y and the
// remainder x - th y taken exactly, then divided by y; within @DIVD@u^2
// (Gappa).
D_MATH_FN struct dj_math_dd
d_math_dd_div_d(
    struct dj_math_dd _x,
    double            _y
)
{
    const double            th = _x.high / _y;
    const struct dj_math_dd p  = d_math_dd_two_product(th, _y);
    // xh - th y's high part is exact, and so is the difference of its rest
    const double            dh = _x.high - p.high;
    const double            dt = dh - p.low;

    return d_math_dd_fast_two_sum(th, (dt + _x.low) / _y);
}

// d_math_dd_div
//   _x / _y: th = xh / yh, the remainder x - th y as a pair, by
// d_math_dd_mul_d and d_math_dd_sub, then divided by yh; within @DIV@u^2
// (Gappa).
D_MATH_FN struct dj_math_dd
d_math_dd_div(
    struct dj_math_dd _x,
    struct dj_math_dd _y
)
{
    const double            th = _x.high / _y.high;
    const struct dj_math_dd r  = d_math_dd_sub(_x, d_math_dd_mul_d(_y, th));

    return d_math_dd_fast_two_sum(th, r.high / _y.high);
}


// 3.4    The square root
//------------------------------------------------------------------------------

// d_math_dd_sqrt
//   the square root of _x >= 0: s = sqrt(xh), correctly rounded
// (elementary.h), then one Newton step, (x - s^2) / (2s), with s^2 taken
// exactly; within @SQRT@u^2 (Gappa). sqrt of 0 is 0, of a negative or NaN
// high part NaN.
D_MATH_FN struct dj_math_dd
d_math_dd_sqrt(
    struct dj_math_dd _x
)
{
    struct dj_math_dd result = { 0.0, 0.0 };

    if (!(_x.high > 0.0))
    {
        result.high = (_x.high == 0.0) ? _x.high : d_math_d_sqrt_c(-1.0);

        return result;
    }

    {
        const double            s  = d_math_d_sqrt_c(_x.high);
        const struct dj_math_dd sq = d_math_dd_two_product(s, s);
        // xh - s^2's high part is exact
        const double            e  = ((_x.high - sq.high) - sq.low) + _x.low;

        return d_math_dd_fast_two_sum(s, e / (2.0 * s));
    }
}
"""

DD_TAIL = r"""
#endif  // DJINTERP_C_MATH_DD_H
"""


def dd():
    lines = [
        "Pairs of doubles: a value held as high + low, unevaluated, |low| at",
        "most half an ulp of high: some 106 significant bits, twice double's,",
        "the double-double arithmetic that the double family's correctly",
        "rounded functions are built on.",
        "  GENERATED by scripts/gen_c_math_float.py: edit the generator, not",
        "this file.",
        "",
        "  Every function is pure double arithmetic, a constant expression from",
        "C++14: the exact sum and product of two doubles, and the sum,",
        "difference, product, quotient and square root of pairs, each within a",
        "stated bound of the exact result, relatively, in units of u^2 =",
        "2^-106, where no operation overflows or underflows. The sums and",
        "products are Joldes, Muller and Popescu's (ACM TOMS 44(2), 2017),",
        "with their proven bounds; the quotients and the square root have",
        "bounds proven with Gappa in chunk 51's record. Each holds whether or",
        "not the compiler fuses a multiply and an add: an exact product takes",
        "its error with an explicit fused multiply-add wherever the target has",
        "one, and no error-free transform takes a product that is not exact",
        "(math_common.h, section 5). On a target that evaluates double wider",
        "than double, a 32-bit x87, no bound holds: build with -msse2",
        "-mfpmath=sse.",
    ]
    text = banner("dd.h", lines, "2026.11.16", "2026.11.21")
    text += "\n" + DD_HEAD + DD_BODY.replace("@DIVD@", DD_BOUNDS["div_d"]).replace(
        "@DIV@", DD_BOUNDS["div"]).replace("@SQRT@", DD_BOUNDS["sqrt"])
    text += DD_TAIL
    return text
