"""gen_c_math.tangent - the text of tangent.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section)


# -----------------------------------------------------------------------------
# tangent.h
# -----------------------------------------------------------------------------
# As exponential.h's: kernels written once, in long double, built on
# elementary.h's, and every family rounding a kernel's pair once.

TANGENT_KERNELS = r"""
//==============================================================================
// 1.  THE KERNELS (long double)
//==============================================================================
// Every family's tangent and arc-tangent are computed here, in long double,
// as exact pairs - elementary.h's (its section 1.1) - and rounded once by
// the family: a float or double result is the kernel's value rounded once
// more, a long double result is the kernel's value. Nothing depends on long
// double's format beyond <float.h>.


// 1.1    The tangent's core
//------------------------------------------------------------------------------
// tan x = sin r / cos r where x's quadrant is even and -cos r / sin r where
// it is odd, r + r_lo being x reduced by pi/2 - exactly, however large x is
// - and the sine and cosine its pairs (elementary.h, sections 1.3 and 1.4).
// The quotient is taken with its residual, num - q den, whose first product
// is exact by Dekker's method, so that it keeps the pairs' precision.

// d_internal_math_ld_tan_of
//   tan(_x) as high + low, _x finite.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_tan_of(
    long double _x
)
{
    const struct dj_internal_math_ld_sin_cos sc =
        d_internal_math_ld_sin_cos_of(_x);
    const bool                            odd = ((sc.quadrant % 2) != 0);
    const struct dj_internal_math_ld_pair num = odd ? sc.cosine : sc.sine;
    const struct dj_internal_math_ld_pair den = odd ? sc.sine : sc.cosine;
    const long double                     q   = num.high / den.high;
    const struct dj_internal_math_ld_pair qd  =
        d_internal_math_ld_product_exact(q, den.high);
    // num - q den: num.high - qd.high is exact (Sterbenz's lemma)
    const long double q_lo = ( ((num.high - qd.high) - qd.low) +
                               (num.low - (q * den.low)) ) / den.high;
    struct dj_internal_math_ld_pair result =
        d_internal_math_ld_two_sum(q, q_lo);

    // an odd quadrant's quotient is negated
    if (odd)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// 1.2    The arc-tangent's core
//------------------------------------------------------------------------------
// atan(-x) = -atan x, and past 1 atan x = pi/2 - atan u, u = 1/x carried as
// u + u_lo (its residual 1 - u x by Dekker's product, until x is so large
// that u is below pi/2's last place); then elementary.h's arc-tangent of the
// unit interval (its section 1.5).

// d_internal_math_ld_atan_of
//   atan(_x) as high + low, _x not NaN; an infinity gives +-pi/2.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_atan_of(
    long double _x
)
{
    const bool        negative = (_x < 0.0L);
    const long double ax       = negative ? -_x : _x;
    const bool        inverted = (ax > 1.0L);
    long double       u_hi     = ax;
    long double       u_lo     = 0.0L;
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // past 1: u = 1/x, and below x = 2^(P+8) its residual (u x stays far
    // from overflow there; past it u is below pi/2's last place)
    if (inverted)
    {
        u_hi = 1.0L / ax;

        if (ax < d_internal_math_ld_pow2(LDBL_MANT_DIG + 8))
        {
            const struct dj_internal_math_ld_pair ux =
                d_internal_math_ld_product_exact(u_hi, ax);

            u_lo = ((1.0L - ux.high) - ux.low) / ax;
        }
    }

    // atan u, or pi/2 - atan u past 1
    result = d_internal_math_ld_atan_reduced(u_hi, u_lo, inverted ? 1 : 0,
                                             inverted);

    if (negative)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// 1.3    The two-argument arc-tangent's core
//------------------------------------------------------------------------------
// atan2(y, x) for y and x finite and nonzero: the smaller magnitude divided
// into the larger as a pair (elementary.h's ratio), then the quadrant from
// the signs - atan u, pi - atan u, or pi/2 -+ atan u past the diagonal - by
// elementary.h's arc-tangent of the unit interval, and y's sign last,
// atan2(-y, x) being -atan2(y, x). No quotient past 1 is formed, so none can
// overflow, for any two finite values.

// d_internal_math_ld_atan2_of
//   atan2(_y, _x) as high + low, _y and _x finite and nonzero.
D_MATH_FN struct dj_internal_math_ld_pair
d_internal_math_ld_atan2_of(
    long double _y,
    long double _x
)
{
    const long double ay       = (_y < 0.0L) ? -_y : _y;
    const long double ax       = (_x < 0.0L) ? -_x : _x;
    // past the diagonal, x / y and a quarter turn
    const bool        steep    = (ay > ax);
    const struct dj_internal_math_ld_pair u =
        steep ? d_internal_math_ld_ratio(ax, 0.0L, ay, 0.0L)
              : d_internal_math_ld_ratio(ay, 0.0L, ax, 0.0L);
    // atan u for x > 0, pi - atan u for x < 0; past the diagonal pi/2 -
    // atan u for x > 0 and pi/2 + atan u for x < 0
    const int         quarters = steep ? 1 : ((_x < 0.0L) ? 2 : 0);
    const bool        minus    = steep ? (_x > 0.0L) : (_x < 0.0L);
    struct dj_internal_math_ld_pair result =
        d_internal_math_ld_atan_reduced(u.high, u.low, quarters, minus);

    if (_y < 0.0L)
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}
"""

TANGENT_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Tangent
//------------------------------------------------------------------------------
// tan_c is the tangent in pure arithmetic, a constant expression from C++14,
// at every finite argument: the kernel's pair (section 1.1) rounded once to
// the family (elementary.h's round_pair). NaN and an infinity give NaN; -0
// stays -0. tan is the C library's.

D_MATH_FN @T@
d_math_@S@_tan_c(
    @T@ _x
)
{
    // NaN and the infinities
    if (!( (_x <= @MAX@) &&
           (_x >= -@MAX@) ))
    {
        return d_internal_math_@S@_nan();
    }

    // zero, of either sign, is its own tangent
    if (_x == (@T@)0)
    {
        return _x;
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_tan_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_tan(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::tan(_x);
#else
    return @CTAN@(_x);
#endif
}

// @SECTION@.2    Arc-tangent
//------------------------------------------------------------------------------
// atan_c is the arc-tangent in pure arithmetic, a constant expression from
// C++14: the kernel's pair (section 1.2) rounded once to the family. An
// infinity gives +-pi/2, rounded to the family; NaN gives NaN; -0 stays -0.
// atan is the C library's.

D_MATH_FN @T@
d_math_@S@_atan_c(
    @T@ _x
)
{
    // NaN, and zero of either sign, are their own arc-tangents
    if ( (_x != _x) ||
         (_x == (@T@)0) )
    {
        return _x;
    }

    {
        const struct dj_internal_math_ld_pair result =
            d_internal_math_ld_atan_of((long double)_x);

        return d_internal_math_@S@_round_pair(result.high, result.low);
    }
}

D_MATH_FN_RT @T@
d_math_@S@_atan(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::atan(_x);
#else
    return @CATAN@(_x);
#endif
}

// @SECTION@.3    Two-argument arc-tangent
//------------------------------------------------------------------------------
// atan2_c is the angle of (_x, _y) in pure arithmetic, a constant expression
// from C++14: the kernel's pair (section 1.3) rounded once to the family, and
// the zeros and infinities as C99's Annex F has them - atan2(+-0, x) is +-0
// for x +0 or positive and +-pi for x -0 or negative; a zero x, or an
// infinite y against a finite x, gives +-pi/2; an infinite y against an
// infinite x +-pi/4, or +-3pi/4 for -inf; a finite y against an infinite x
// +-0, or +-pi for -inf. NaN in either gives NaN. atan2 is the C library's.

// d_internal_math_@S@_negative
//   whether _x is negative, -0 among them. No comparison sees a zero's sign:
// GCC and Clang read it with copysign, a builtin either evaluates in a
// constant expression; elsewhere it is 1 / _x, -inf for -0 at run time but no
// constant expression.
D_MATH_FN bool
d_internal_math_@S@_negative(
    @T@ _x
)
{
    if (_x != (@T@)0)
    {
        return (_x < (@T@)0);
    }

#if ( (defined(D_ENV_COMPILER_GCC)) ||                                         \
      (defined(D_ENV_COMPILER_CLANG)) )
    return (@COPYSIGN@((@T@)1, _x) < (@T@)0);
#else
    return (((@T@)1 / _x) < (@T@)0);
#endif
}

// d_internal_math_@S@_turns
//   _quarters pi/2 + atan(_u), or less it when _minus is true, rounded once
// to the family and negated when _negative is true: atan2's values at the
// zeros and infinities - pi/2, pi, pi/4, 3pi/4 - by the kernel of the rest.
D_MATH_FN @T@
d_internal_math_@S@_turns(
    long double _u,
    int         _quarters,
    bool        _minus,
    bool        _negative
)
{
    const struct dj_internal_math_ld_pair angle =
        d_internal_math_ld_atan_reduced(_u, 0.0L, _quarters, _minus);
    const @T@ value =
        d_internal_math_@S@_round_pair(angle.high, angle.low);

    return _negative ? -value
                     : value;
}

D_MATH_FN @T@
d_math_@S@_atan2_c(
    @T@ _y,
    @T@ _x
)
{
    // NaN in either
    if (_y != _y)
    {
        return _y;
    }

    if (_x != _x)
    {
        return _x;
    }

    {
        const bool y_negative = d_internal_math_@S@_negative(_y);
        const bool x_negative = d_internal_math_@S@_negative(_x);
        const bool y_infinite = !( (_y <= @MAX@) &&
                                   (_y >= -@MAX@) );
        const bool x_infinite = !( (_x <= @MAX@) &&
                                   (_x >= -@MAX@) );

        // y = +-0: +-0 toward +0 and the positives, +-pi toward -0 and the
        // negatives
        if (_y == (@T@)0)
        {
            return x_negative ? d_internal_math_@S@_turns(0.0L, 2, false,
                                                          y_negative)
                              : _y;
        }

        // x = +-0, or y infinite and x finite: +-pi/2
        if ( (_x == (@T@)0) ||
             ( (y_infinite) &&
               (!x_infinite) ) )
        {
            return d_internal_math_@S@_turns(0.0L, 1, false, y_negative);
        }

        // both infinite: +-pi/4, or +-3pi/4 toward -inf
        if (y_infinite)
        {
            return x_negative ? d_internal_math_@S@_turns(1.0L, 2, true,
                                                          y_negative)
                              : d_internal_math_@S@_turns(1.0L, 0, false,
                                                          y_negative);
        }

        // y finite, x infinite: +-0, or +-pi toward -inf
        if (x_infinite)
        {
            return x_negative ? d_internal_math_@S@_turns(0.0L, 2, false,
                                                          y_negative)
                              : (y_negative ? -(@T@)0
                                            : (@T@)0);
        }

        {
            const struct dj_internal_math_ld_pair result =
                d_internal_math_ld_atan2_of((long double)_y,
                                            (long double)_x);

            return d_internal_math_@S@_round_pair(result.high, result.low);
        }
    }
}

D_MATH_FN_RT @T@
d_math_@S@_atan2(
    @T@ _y,
    @T@ _x
)
{
#ifdef __cplusplus
    return std::atan2(_y, _x);
#else
    return @CATAN2@(_y, _x);
#endif
}
"""

TANGENT_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE KERNELS (long double)
    -------------------------
    1.  The tangent's core
    2.  The arc-tangent's core
    3.  The two-argument arc-tangent's core
2.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Tangent
    2.  Arc-tangent
    3.  Two-argument arc-tangent
3.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Tangent
    2.  Arc-tangent
    3.  Two-argument arc-tangent
4.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Tangent
    2.  Arc-tangent
    3.  Two-argument arc-tangent
5.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_TANGENT_H
#define DJINTERP_C_MATH_TANGENT_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::tan, std::atan, std::atan2
#endif  // __cplusplus
#include <float.h>           // FLT_MAX ... LDBL_MAX, LDBL_MANT_DIG
#include <math.h>            // tanf ... atan2l
// djinterp
#include "./elementary.h"    // d_internal_math_ld_sin_cos_of, *_atan_reduced
#include "./float_order.h"   // d_internal_math_ld_pow2
#include "./math_common.h"   // D_MATH_FN
"""

TANGENT_TAIL = r"""

#endif  // DJINTERP_C_MATH_TANGENT_H
"""

TANGENT_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_tan_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_tan", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_atan_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_atan", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_atan2_c", [("@T@", "_y"), ("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_atan2", [("@T@", "_y"), ("@T@", "_x")]),
]


def tangent():
    lines = [
        "Tangent and arc-tangent: the C core's tan, atan and atan2, for float,",
        "double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. The _c",
        "forms are computed once, in long double, as exact pairs, and rounded once",
        "to the family: the correctly rounded value but where the exact one lies",
        "within the kernel's error of a midpoint, and so within an ulp of the",
        "library's. tan_c reduces its argument by pi/2 exactly at every finite",
        "value, as elementary.h's sine and cosine do.",
    ]
    text = banner("tangent.h", lines, "2026.10.09", "2026.11.21")
    text += "\n" + TANGENT_HEAD
    text += TANGENT_KERNELS
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = TANGENT_TEMPLATE.replace("@SECTION@", str(i + 2)).replace("@UPPER@", upper)
        body = body.replace("@CTAN@", {"float": "tanf", "double": "tan", "long double": "tanl"}[ctype])
        body = body.replace("@CATAN2@", {"float": "atan2f", "double": "atan2", "long double": "atan2l"}[ctype])
        body = body.replace("@CATAN@", {"float": "atanf", "double": "atan", "long double": "atanl"}[ctype])
        body = body.replace("@COPYSIGN@", {"float": "__builtin_copysignf", "double": "__builtin_copysign", "long double": "__builtin_copysignl"}[ctype])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("5.  ONE NAME FOR EVERY FAMILY (C++)", TANGENT_OVERLOADS)
    text += TANGENT_TAIL
    return text
