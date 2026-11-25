"""gen_c_math.nearest_integer - the text of nearest_integer.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section)


# -----------------------------------------------------------------------------
# nearest_integer.h
# -----------------------------------------------------------------------------
# C's nearest integer functions (7.12.9) that do not read the rounding mode:
# floor, ceil, trunc and round. Exact - each is a value of the family - and
# right over the whole range, past 2^63 too (N5).

NEAREST_INTEGER_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_@S@_own_integer
//   true for a value that floor, ceil, trunc and round all leave as it is:
// NaN, a zero, and from 2^(P-1) up, where every value is an integer - the
// infinities among them. 2^(P-1) is 1 / @EPS@, exactly.
D_MATH_FN bool
d_internal_math_@S@_own_integer(
    @T@ _x
)
{
    const @T@ top = (@T@)1 / @EPS@;

    return ( (_x != _x)     ||
             (_x == (@T@)0) ||
             (!(_x < top))  ||
             (!(_x > -top)) );
}

// d_internal_math_@S@_whole
//   the integer part of _a, 0 <= _a < 2^(P-1), exactly. A conversion to
// dj_math_umax truncates exactly any value below 2^W, W its bits: every _a,
// where P - 1 <= W. Past 2^W - a quad's long double, or a family under a
// 32-bit dj_math_umax - _a is taken W bits at a time from the top: each part
// a whole multiple of a power of 2^W, each difference exact.
D_MATH_FN @T@
d_internal_math_@S@_whole(
    @T@ _a
)
{
    // 2^W: 2^(W-1) is exact as an integer and as a value of the family, and
    // so is twice it
    const @T@ span =
        (@T@)((D_MATH_UMAX_MAX >> 1) + 1u) * (@T@)2;

    if (_a < span)
    {
        return (@T@)(dj_math_umax)_a;
    }

    {
        @T@ scale = span;
        @T@ whole = (@T@)0;
        @T@ rest  = _a;

        // the largest power of 2^W at or below _a
        while (!(rest < (scale * span)))
        {
            scale *= span;
        }

        // then its digits, from the top, each below 2^W
        for (;;)
        {
            const @T@ part =
                (@T@)(dj_math_umax)(rest / scale) * scale;

            whole += part;
            rest  -= part;

            if (scale == (@T@)1)
            {
                break;
            }

            scale /= span;
        }

        return whole;
    }
}

// @SECTION@.2    Nearest integers
//------------------------------------------------------------------------------
// floor_c, ceil_c, trunc_c and round_c are C's floor, ceil, trunc and round,
// exactly - the integer each picks is a value of the family - in pure
// arithmetic, a constant expression from C++14, over the whole range. round
// takes a half away from zero. A zero keeps its sign, and so does a result
// that is a zero: ceil(-0.5), trunc(-0.5) and round(-0.25) are -0. The
// infinities are their own; NaN gives NaN, the argument itself. The plain
// forms are the C library's, exact too: the two agree bit for bit. C++
// before C++11 has no trunc or round, so there those are the _c forms.

D_MATH_FN @T@
d_math_@S@_floor_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);

        if (_x > (@T@)0)
        {
            return w;
        }

        // below zero, a step further down unless the value is whole
        return (w == a) ? _x : -(w + (@T@)1);
    }
}

D_MATH_FN @T@
d_math_@S@_ceil_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);

        // below zero the integer part, -0 for a fraction
        if (_x < (@T@)0)
        {
            return -w;
        }

        return (w == a) ? _x : (w + (@T@)1);
    }
}

D_MATH_FN @T@
d_math_@S@_trunc_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);

        return (_x < (@T@)0) ? -w : w;
    }
}

D_MATH_FN @T@
d_math_@S@_round_c(
    @T@ _x
)
{
    if (d_internal_math_@S@_own_integer(_x))
    {
        return _x;
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ w = d_internal_math_@S@_whole(a);
        // the fraction a - w is exact: a half or more goes away from zero
        const @T@ r =
            ((a - w) < (@T@)0.5) ? w : (w + (@T@)1);

        return (_x < (@T@)0) ? -r : r;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_floor(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::floor(_x);
#else
    return @CFLOOR@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_ceil(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::ceil(_x);
#else
    return @CCEIL@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_trunc(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::trunc(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no trunc: the pure-arithmetic form serves
    return d_math_@S@_trunc_c(_x);
#else
    return @CTRUNC@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_round(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::round(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no round: the pure-arithmetic form serves
    return d_math_@S@_round_c(_x);
#else
    return @CROUND@(_x);
#endif
}
"""

NEAREST_INTEGER_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Nearest integers
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Nearest integers
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Nearest integers
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_NEAREST_INTEGER_H
#define DJINTERP_C_MATH_NEAREST_INTEGER_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::floor, std::ceil, std::trunc, std::round
#endif  // __cplusplus
#include <float.h>           // FLT_EPSILON ... LDBL_EPSILON
#include <math.h>            // floorf ... roundl
// djinterp
#include "./math_common.h"   // D_MATH_FN, dj_math_umax, D_MATH_UMAX_MAX
"""

NEAREST_INTEGER_TAIL = r"""

#endif  // DJINTERP_C_MATH_NEAREST_INTEGER_H
"""

NEAREST_INTEGER_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_floor_c", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_ceil_c", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_trunc_c", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_round_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_floor", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_ceil", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_trunc", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_round", [("@T@", "_x")]),
]


def nearest_integer():
    lines = [
        "Nearest integers: the C core's floor, ceil, trunc and round, for",
        "float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. Each",
        "is exact - the integer it picks is a value of the family - and right",
        "over the whole range: the integer part is taken by conversion to",
        "dj_math_umax, never long long, a wide value's a part at a time, and",
        "from 2^(P-1) up every value is its own. So the _c forms agree with the",
        "library bit for bit, a zero's sign included, and a face may use",
        "either and not know which ran. C++ before C++11 has no trunc or round,",
        "so that there those are the _c forms.",
    ]
    text = banner("nearest_integer.h", lines, "2026.10.11", "2026.11.21")
    text += "\n" + NEAREST_INTEGER_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = NEAREST_INTEGER_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        for key, names in (("@CFLOOR@", ("floorf", "floor", "floorl")),
                           ("@CCEIL@", ("ceilf", "ceil", "ceill")),
                           ("@CTRUNC@", ("truncf", "trunc", "truncl")),
                           ("@CROUND@", ("roundf", "round", "roundl"))):
            body = body.replace(key, names[("float", "double", "long double").index(ctype)])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", NEAREST_INTEGER_OVERLOADS)
    text += NEAREST_INTEGER_TAIL
    return text
