"""gen_c_math.scalar - the text of scalar.h (chunk 64, ruling 13).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section)


# -----------------------------------------------------------------------------
# scalar.h
# -----------------------------------------------------------------------------
# The scalar helpers the terps call, as the owner's ruling 13 has them (B,
# 2026.11.24): abs, copysign and is_finite; min and max as IEEE 754-2019's
# minimum and maximum, a NaN in giving NaN out and -0 below +0, with min_num
# and max_num, which ignore a NaN, as C23 has both; clamp, which keeps a
# NaN; lerp, exact at both ends and never past them, as C++20's is; and
# ldexp and frexp. Each is exact, so each answers to the bit everywhere.

SCALAR_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    Internal helpers
//------------------------------------------------------------------------------

// d_internal_math_@S@_sign_set
//   whether _x's sign bit is set: a negative value, -0, or a NaN whose sign
// is set. No comparison sees a zero's sign or a NaN's: GCC and Clang read it
// with copysign, a builtin either evaluates in a constant expression;
// elsewhere a NaN counts as positive, and -0 is told by 1 / _x, -inf for -0
// at run time but no constant expression (as tangent.h's negative is).
D_MATH_FN bool
d_internal_math_@S@_sign_set(
    @T@ _x
)
{
#if ( (defined(D_ENV_COMPILER_GCC)) ||                                         \
      (defined(D_ENV_COMPILER_CLANG)) )
    return (@COPYSIGN_BUILTIN@((@T@)1, _x) < (@T@)0);
#else
    if (_x != _x)
    {
        return false;
    }

    if (_x != (@T@)0)
    {
        return (_x < (@T@)0);
    }

    return (((@T@)1 / _x) < (@T@)0);
#endif
}

// @SECTION@.2    Sign and finiteness
//------------------------------------------------------------------------------
// abs_c is |_x|, +0 for either zero, a NaN with its sign cleared; copysign_c
// is _x's magnitude with _y's sign, a zero's and a NaN's sign among them;
// is_finite_c is true for every value but the infinities and NaN. Each is
// pure arithmetic, a constant expression from C++14; the plain forms are the
// C library's fabs, copysign and isfinite, and the two agree bit for bit.
// C++ before C++11 has no copysign or isfinite, so there those are the _c
// forms.

D_MATH_FN @T@
d_math_@S@_abs_c(
    @T@ _x
)
{
    // negating flips the sign bit alone, a NaN's too
    return d_internal_math_@S@_sign_set(_x) ? -_x : _x;
}

D_MATH_FN @T@
d_math_@S@_copysign_c(
    @T@ _x,
    @T@ _y
)
{
    return (d_internal_math_@S@_sign_set(_x) !=
            d_internal_math_@S@_sign_set(_y)) ? -_x : _x;
}

D_MATH_FN bool
d_math_@S@_is_finite_c(
    @T@ _x
)
{
    // comparisons alone, which a constant expression allows at an infinity
    // and at NaN, false for NaN
    return ( (_x >= -@MAX@) &&
             (_x <= @MAX@) );
}

D_MATH_FN_RT @T@
d_math_@S@_abs(
    @T@ _x
)
{
#ifdef __cplusplus
    return std::fabs(_x);
#else
    return @CFABS@(_x);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_copysign(
    @T@ _x,
    @T@ _y
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::copysign(_x, _y);
#elif defined(__cplusplus)
    // C++ before C++11 has no copysign: the pure-arithmetic form serves
    return d_math_@S@_copysign_c(_x, _y);
#else
    return @CCOPYSIGN@(_x, _y);
#endif
}

D_MATH_FN_RT bool
d_math_@S@_is_finite(
    @T@ _x
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::isfinite(_x);
#elif defined(__cplusplus)
    // C++ before C++11 has no isfinite: the pure-arithmetic form serves
    return d_math_@S@_is_finite_c(_x);
#else
    return (isfinite(_x) != 0);
#endif
}

// @SECTION@.3    The least and the greatest
//------------------------------------------------------------------------------
// min and max are IEEE 754-2019's minimum and maximum, as the expression
// language's are (S338): a NaN in either gives that NaN, the first where
// both are, and -0 is less than +0. min_num and max_num are its
// minimumNumber and maximumNumber: a NaN is passed over for the other,
// NaN only where both are, and -0 less than +0 as before. C23 has both
// pairs, fminimum and fminimum_num; C99's fmin and fmax lose a NaN and
// leave a zero's sign open. clamp is _x held to [_lo, _hi], max then min:
// a NaN in any of the three gives NaN, and a _lo above _hi gives _hi. Each
// is exact, pure arithmetic, a constant expression from C++14, with one
// form.

D_MATH_FN @T@
d_math_@S@_min(
    @T@ _x,
    @T@ _y
)
{
    // a NaN: the first, itself
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    if (_x < _y)
    {
        return _x;
    }

    if (_y < _x)
    {
        return _y;
    }

    // equal: two zeros give -0 where either is -0
    return d_internal_math_@S@_sign_set(_x) ? _x : _y;
}

D_MATH_FN @T@
d_math_@S@_max(
    @T@ _x,
    @T@ _y
)
{
    // a NaN: the first, itself
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    if (_x > _y)
    {
        return _x;
    }

    if (_y > _x)
    {
        return _y;
    }

    // equal: two zeros give +0 where either is +0
    return d_internal_math_@S@_sign_set(_x) ? _y : _x;
}

D_MATH_FN @T@
d_math_@S@_min_num(
    @T@ _x,
    @T@ _y
)
{
    // a NaN passed over: _y itself, a NaN only where both are
    if (_x != _x)
    {
        return _y;
    }

    if (_y != _y)
    {
        return _x;
    }

    return d_math_@S@_min(_x, _y);
}

D_MATH_FN @T@
d_math_@S@_max_num(
    @T@ _x,
    @T@ _y
)
{
    // a NaN passed over: _y itself, a NaN only where both are
    if (_x != _x)
    {
        return _y;
    }

    if (_y != _y)
    {
        return _x;
    }

    return d_math_@S@_max(_x, _y);
}

D_MATH_FN @T@
d_math_@S@_clamp(
    @T@ _x,
    @T@ _lo,
    @T@ _hi
)
{
    return d_math_@S@_min(d_math_@S@_max(_x, _lo), _hi);
}

// @SECTION@.4    Interpolation
//------------------------------------------------------------------------------
// lerp is _a + _t (_b - _a) as C++20's std::lerp has it, after Herring's
// paper P0811: exactly _a at _t = 0 and _b at _t = 1, _a where _a and _b
// are equal, monotonic in _t, and for _t in [0, 1] never outside [_a, _b].
// Ends of opposite signs, or a zero end, take _t _b + (1 - _t) _a, whose
// two products cannot cancel; others take _a + _t (_b - _a), held at _b on
// the side _t lies on. A NaN in any gives NaN, the first. One form, pure
// arithmetic, a constant expression from C++14; no product is fused in
// this header (its pragma, and ISO modes' own), so it answers to the bit.

D_MATH_FN @T@
d_math_@S@_lerp(
    @T@ _a,
    @T@ _b,
    @T@ _t
)
{
    // a NaN: the first, itself
    if (_a != _a)
    {
        return _a;
    }

    if (_b != _b)
    {
        return _b;
    }

    if (_t != _t)
    {
        return _t;
    }

    // ends of opposite signs, or a zero end: both products, no cancelling
    if ( ( (_a <= (@T@)0) && (_b >= (@T@)0) ) ||
         ( (_a >= (@T@)0) && (_b <= (@T@)0) ) )
    {
        return (_t * _b) + (((@T@)1 - _t) * _a);
    }

    // the far end exactly
    if (_t == (@T@)1)
    {
        return _b;
    }

    {
        const @T@ x = _a + (_t * (_b - _a));

        // held at _b: below it where _t moves toward it, above it past it
        if ((_t > (@T@)1) == (_b > _a))
        {
            return (_b < x) ? x : _b;
        }

        return (x < _b) ? x : _b;
    }
}

// @SECTION@.5    Powers of two
//------------------------------------------------------------------------------
// ldexp_c is _x 2^_n, rounded once - exact but where it overflows or falls
// among the subnormals - and frexp_c the fraction in [1/2, 1) and exponent
// _x is made of, the fraction with _x's sign: a zero, an infinity and NaN
// give themselves and exponent 0. Pure arithmetic, a constant expression
// from C++14, by float_order.h's exact powers; the plain forms are the C
// library's ldexp and frexp, exact too, and the two agree bit for bit.

D_MATH_FN @T@
d_math_@S@_ldexp_c(
    @T@ _x,
    int _n
)
{
    // a zero, an infinity, NaN: itself
    if ( (_x == (@T@)0) ||
         (!d_math_@S@_is_finite_c(_x)) )
    {
        return _x;
    }

    {
        // 2^top, the largest power of two; 2^bottom, the least normal one
        const int top    = @EMAX@ - 1;
        const int bottom = @EMIN@ - 1;
        @T@       y      = _x;
        int       n      = _n;

        // up: at most two steps of 2^top before the last, past which every
        // finite value has overflowed
        if (n > top)
        {
            y *= d_internal_math_@S@_pow2(top);
            n -= top;

            if (n > top)
            {
                y *= d_internal_math_@S@_pow2(top);
                n -= top;

                if (n > top)
                {
                    n = top;
                }
            }
        }
        // down: steps of 2^(bottom + P), which leave the last step at least
        // P binades below the least normal - so that where a step's value
        // falls among the subnormals, the true result lies below half the
        // least subnormal, and the one rounding that counts, to a zero, is
        // the last
        else if (n < bottom)
        {
            y *= d_internal_math_@S@_pow2(bottom + @P@);
            n -= bottom + @P@;

            if (n < bottom)
            {
                y *= d_internal_math_@S@_pow2(bottom + @P@);
                n -= bottom + @P@;

                if (n < bottom)
                {
                    n = bottom;
                }
            }
        }

        // the last step, 2^n a normal power: the one rounding
        return y * d_internal_math_@S@_pow2(n);
    }
}

D_MATH_FN @T@
d_math_@S@_frexp_c(
    @T@  _x,
    int* _exponent
)
{
    // a zero, an infinity, NaN: itself, exponent 0
    if ( (_x == (@T@)0) ||
         (!d_math_@S@_is_finite_c(_x)) )
    {
        *_exponent = 0;

        return _x;
    }

    {
        @T@ a     = (_x < (@T@)0) ? -_x : _x;
        int shift = 0;

        // a subnormal: up by 2^P first, exactly, into the normal range
        if (a < @MIN@)
        {
            a     *= d_internal_math_@S@_pow2(@P@);
            shift  = @P@;
        }

        {
            const struct dj_internal_math_@S@_binade b =
                d_internal_math_@S@_binade_of(a);
            // a / 2^E lies in [1, 2), exactly: its half is the fraction
            const @T@ fraction = (a / b.power) / (@T@)2;

            *_exponent = b.exponent + 1 - shift;

            return (_x < (@T@)0) ? -fraction : fraction;
        }
    }
}

D_MATH_FN_RT @T@
d_math_@S@_ldexp(
    @T@ _x,
    int _n
)
{
#ifdef __cplusplus
    return std::ldexp(_x, _n);
#else
    return @CLDEXP@(_x, _n);
#endif
}

D_MATH_FN_RT @T@
d_math_@S@_frexp(
    @T@  _x,
    int* _exponent
)
{
#ifdef __cplusplus
    return std::frexp(_x, _exponent);
#else
    return @CFREXP@(_x, _exponent);
#endif
}
"""

SCALAR_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  Internal helpers
    2.  Sign and finiteness
    3.  The least and the greatest
    4.  Interpolation
    5.  Powers of two
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  Internal helpers
    2.  Sign and finiteness
    3.  The least and the greatest
    4.  Interpolation
    5.  Powers of two
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  Internal helpers
    2.  Sign and finiteness
    3.  The least and the greatest
    4.  Interpolation
    5.  Powers of two
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_SCALAR_H
#define DJINTERP_C_MATH_SCALAR_H 1

// std
#ifdef __cplusplus
    #include <cmath>           // std::fabs, std::copysign, std::isfinite,
                               // std::ldexp, std::frexp
#endif  // __cplusplus
#include <float.h>             // FLT_MIN ... LDBL_MAX_EXP
#include <math.h>              // fabsf ... frexpl, isfinite
// djinterp
#include "./math_common.h"     // D_MATH_FN, D_MATH_FN_RT
#include "./float_order.h"     // d_internal_math_*_pow2, _binade_of
"""

SCALAR_TAIL = r"""

#endif  // DJINTERP_C_MATH_SCALAR_H
"""

SCALAR_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_abs_c", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_copysign_c", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "bool", "d_math_is_finite_c", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_abs", [("@T@", "_x")]),
    ("D_MATH_FN_RT", "@T@", "d_math_copysign", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN_RT", "bool", "d_math_is_finite", [("@T@", "_x")]),
    ("D_MATH_FN", "@T@", "d_math_min", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_max", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_min_num", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_max_num", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_clamp",
     [("@T@", "_x"), ("@T@", "_lo"), ("@T@", "_hi")]),
    ("D_MATH_FN", "@T@", "d_math_lerp",
     [("@T@", "_a"), ("@T@", "_b"), ("@T@", "_t")]),
    ("D_MATH_FN", "@T@", "d_math_ldexp_c", [("@T@", "_x"), ("int", "_n")]),
    ("D_MATH_FN", "@T@", "d_math_frexp_c",
     [("@T@", "_x"), ("int*", "_exponent")]),
    ("D_MATH_FN_RT", "@T@", "d_math_ldexp", [("@T@", "_x"), ("int", "_n")]),
    ("D_MATH_FN_RT", "@T@", "d_math_frexp",
     [("@T@", "_x"), ("int*", "_exponent")]),
]

LIBRARY = {
    "@CFABS@": ("fabsf", "fabs", "fabsl"),
    "@CCOPYSIGN@": ("copysignf", "copysign", "copysignl"),
    "@CLDEXP@": ("ldexpf", "ldexp", "ldexpl"),
    "@CFREXP@": ("frexpf", "frexp", "frexpl"),
    "@COPYSIGN_BUILTIN@": ("__builtin_copysignf", "__builtin_copysign",
                           "__builtin_copysignl"),
}


def scalar():
    lines = [
        "Scalar helpers: abs, copysign, is_finite, min, max, min_num, max_num,",
        "clamp, lerp, ldexp and frexp, for float, double and long double, as the",
        "owner's ruling 13 has them (2026.11.24).",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each is exact, so each answers to the bit on every platform. min and",
        "max are IEEE 754-2019's minimum and maximum: a NaN in gives NaN out,",
        "and -0 is less than +0, as the expression language's are (S338).",
        "min_num and max_num pass a NaN over for the other number, as C23's",
        "fminimum_num does. clamp keeps a NaN. lerp is exact at both ends and",
        "never past them, as C++20's is. abs, copysign, is_finite, ldexp and",
        "frexp come twice, as in nearest_integer.h: the _c form pure arithmetic,",
        "a constant expression from C++14, and the plain form the C library's,",
        "the two agreeing bit for bit; the rest, which C99 lacks, once.",
    ]
    text = banner("scalar.h", lines, "2026.11.24", "2026.11.24")
    text += "\n" + SCALAR_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        body = SCALAR_TEMPLATE.replace("@SECTION@", str(i + 1)).replace(
            "@UPPER@", prose.upper())
        k = ("float", "double", "long double").index(ctype)
        for key, names in LIBRARY.items():
            body = body.replace(key, names[k])
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)",
                             SCALAR_OVERLOADS)
    text += SCALAR_TAIL
    return text
