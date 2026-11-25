"""gen_c_math.cplx - the text of cplx.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section, re)
from .elementary import (TWO_OVER_PI_DIGITS)
from .cplx_text import (CPLX_ARITH, CPLX_COMPOSED, CPLX_INVERSE,
                        CPLX_KERNELS, CPLX_STRUCT, CPLX_TURNS, CPLX_TYPES)

# the family's functions, each twice: _c on the long double kernel, rounded
# once to the family, and plain on the family's own kernel
CPLX_FUNCTIONS_HEAD = r"""
// @SECTION@.@SUB@    The @NAME@ family (_@S@)
//------------------------------------------------------------------------------
// Each function twice, as every real function in the core: the _c form a
// constant expression from C++14, the long double kernel of pure arithmetic
// (section 3.1) rounded once to the family - a float or double part within
// half an ulp but where the kernel's error reaches a midpoint, a long double
// part within the kernel's few ulps - and the plain form the family's own
// kernel on the C library's real functions (sections 3.2 to 3.4), within a
// few ulps of each part and fast. abs and arg are hypot and atan2 of the
// family themselves.

// d_internal_cplx_@S@_widen, d_internal_cplx_@S@_narrow
//   _z's parts as long doubles, exactly, and a long double result's parts
// rounded once to the family.
D_MATH_FN struct dj_cplx_ld
d_internal_cplx_@S@_widen(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_ld_make((long double)_z.part[0],
                                   (long double)_z.part[1]);
}

D_MATH_FN struct dj_cplx_@S@
d_internal_cplx_@S@_narrow(
    struct dj_cplx_ld _z
)
{
    return d_internal_cplx_@S@_make((@T@)_z.part[0],
                                    (@T@)_z.part[1]);
}

// @SECTION@.@SUB@.1
// d_cplx_@S@_abs_c, d_cplx_@S@_abs, d_cplx_@S@_arg_c, d_cplx_@S@_arg
//   function: the modulus, hypot(x, y), and the argument, atan2(y, x), in
// (-pi, pi], the family's own (C99's cabs and carg): +inf for an infinite
// part, the other a NaN or not.
D_MATH_FN @T@
d_cplx_@S@_abs_c(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_hypot_c(_z.part[0],
                              _z.part[1]);
}

D_MATH_FN_RT @T@
d_cplx_@S@_abs(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_hypot(_z.part[0],
                            _z.part[1]);
}

D_MATH_FN @T@
d_cplx_@S@_arg_c(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_atan2_c(_z.part[1],
                              _z.part[0]);
}

D_MATH_FN_RT @T@
d_cplx_@S@_arg(
    struct dj_cplx_@S@ _z
)
{
    return d_math_@S@_atan2(_z.part[1],
                            _z.part[0]);
}
"""

CPLX_UNARY_DOC = {
    "exp": ("e^z, e^x (cos y + i sin y) (C99's cexp)", 2),
    "log": ("the principal logarithm, log |z| + i arg z, its imaginary part in\n// (-pi, pi] (C99's clog)", 3),
    "sqrt": ("the principal square root, its real part nonnegative (C99's\n// csqrt)", 4),
    "sin": ("the sine, -i sinh(iz) (C99's csin)", 6),
    "cos": ("the cosine, cosh(iz) (C99's ccos)", 6),
    "tan": ("the tangent, -i tanh(iz) (C99's ctan)", 6),
    "asin": ("the principal arc-sine, -i asinh(iz) (C99's casin)", 8),
    "acos": ("the principal arc-cosine, its real part in [0, pi] (C99's\n// cacos)", 8),
    "atan": ("the principal arc-tangent, -i atanh(iz) (C99's catan)", 8),
    "sinh": ("the hyperbolic sine (C99's csinh)", 7),
    "cosh": ("the hyperbolic cosine (C99's ccosh)", 7),
    "tanh": ("the hyperbolic tangent (C99's ctanh)", 7),
    "asinh": ("the principal inverse hyperbolic sine (C99's casinh)", 9),
    "acosh": ("the principal inverse hyperbolic cosine, its real part\n// nonnegative (C99's cacosh)", 9),
    "atanh": ("the principal inverse hyperbolic tangent (C99's catanh)", 9),
}

CPLX_UNARY = r"""
// d_cplx_@S@_@F@_c, d_cplx_@S@_@F@
//   function: @DOC@.
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_@F@_c(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_@F@_c(d_internal_cplx_@S@_widen(_z)));
}

D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_@F@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_@F@(_z);
}
"""

CPLX_POW = r"""
// d_cplx_@S@_pow_c, d_cplx_@S@_pow
//   function: the principal power, exp(w log z) (C99's cpow), with z^0 = 1,
// whole powers by repeated squaring, and the real and imaginary cases exact
// where their value is (section 3's pow).
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_pow_c(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_pow_c(d_internal_cplx_@S@_widen(_z),
                                 d_internal_cplx_@S@_widen(_w)));
}

D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_pow(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    return d_internal_cplx_@S@_pow(_z,
                                   _w);
}
"""

CPLX_COMPLEX = r"""
// @SECTION@.@SUB@    The @NAME@ family (_@S@)
//------------------------------------------------------------------------------
// d_cplx_@S@_from_complex, d_cplx_@S@_to_complex
//   function: a @T@ _Complex as a struct dj_cplx_@S@ and back, by a copy of
// its bytes: the two have one layout (C99 6.2.5).
D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_from_complex(
    @T@ _Complex _z
)
{
    struct dj_cplx_@S@ result = { { (@T@)0, (@T@)0 } };

    memcpy(result.part,
           &_z,
           sizeof(result.part));

    return result;
}

D_MATH_FN_RT @T@ _Complex
d_cplx_@S@_to_complex(
    struct dj_cplx_@S@ _z
)
{
    @T@ _Complex result = (@T@)0;

    memcpy(&result,
           _z.part,
           sizeof(_z.part));

    return result;
}
"""

# the five's docs, whole, and where on the real axis each is the real
# function's: everywhere, x positive, or x past -1
CPLX_FIVE_DOC = {
    "exp2": ("2^z, 2^x (cos(y ln 2) + i sin(y ln 2)), y ln 2 reduced\n"
             "// by pi/2 exactly for every y. On the real axis, the real "
             "function's value\n"
             "// and y's zero, the _c form's _c and the plain form's plain; "
             "else, in both\n"
             "// forms, the long double kernel (section 3.1) rounded once "
             "to the family.", ""),
    "log2": ("the principal base-2 logarithm, log z / ln 2. On the\n"
             "// real axis, x positive, the real function's value and y's "
             "zero, the _c\n"
             "// form's _c and the plain form's plain; else, in both forms, "
             "the long\n"
             "// double kernel (section 3.1) rounded once to the family.",
             "positive"),
    "log10": ("the principal base-10 logarithm, log z / ln 10. On the\n"
              "// real axis, x positive, the real function's value and y's "
              "zero, the _c\n"
              "// form's _c and the plain form's plain; else, in both "
              "forms, the long\n"
              "// double kernel (section 3.1) rounded once to the family.",
              "positive"),
    "log1p": ("the principal logarithm of 1 + z, 1 + z never rounded.\n"
              "// On the real axis, x past -1, the real function's value "
              "and y's zero, the\n"
              "// _c form's _c and the plain form's plain; else, in both "
              "forms, the long\n"
              "// double kernel (section 3.1) rounded once to the family.",
              "past -1"),
    "expm1": ("e^z - 1, without the cancellation of e^z less 1. On\n"
              "// the real axis, the real function's value and y's zero, "
              "the _c form's _c\n"
              "// and the plain form's plain; else, in both forms, the "
              "long double kernel\n"
              "// (section 3.1) rounded once to the family.", ""),
}

CPLX_FIVE = r"""
// d_cplx_@S@_@F@_c, d_cplx_@S@_@F@
//   function: @DOC@
D_MATH_FN struct dj_cplx_@S@
d_cplx_@S@_@F@_c(
    struct dj_cplx_@S@ _z
)
{
    // on the real axis@WHERE@: the real function's own value
    if @AXIS@
    {
        return d_internal_cplx_@S@_make(d_math_@S@_@F@_c(_z.part[0]),
                                        _z.part[1]);
    }

    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_@F@_c(d_internal_cplx_@S@_widen(_z)));
}

D_MATH_FN_RT struct dj_cplx_@S@
d_cplx_@S@_@F@(
    struct dj_cplx_@S@ _z
)
{
    // on the real axis@WHERE@: the real function's own value
    if @AXIS@
    {
        return d_internal_cplx_@S@_make(d_math_@S@_@F@(_z.part[0]),
                                        _z.part[1]);
    }

    return d_internal_cplx_@S@_narrow(
        d_internal_cplx_ld_@F@_c(d_internal_cplx_@S@_widen(_z)));
}
"""

CPLX_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE TYPES
    ---------
    1.  The complex types
         1.  dj_cplx_f
         2.  dj_cplx_d
         3.  dj_cplx_ld
2.  THE ARITHMETIC
    --------------
    1.  The float family (_f)
    2.  The double family (_d)
    3.  The long double family (_ld)
3.  THE KERNELS
    -----------
    1.  Pure arithmetic, in long double
    2.  On the C library, in float
    3.  On the C library, in double
    4.  On the C library, in long double
4.  THE FUNCTIONS
    -------------
    1.  The float family (_f)
    2.  The double family (_d)
    3.  The long double family (_ld)
5.  C99'S _Complex (C ONLY)
    -----------------------
    1.  The float family (_f)
    2.  The double family (_d)
    3.  The long double family (_ld)
6.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_CPLX_H
#define DJINTERP_C_MATH_CPLX_H 1

// std
#include <float.h>               // FLT_MAX ... LDBL_MAX, LDBL_MANT_DIG
#include <string.h>              // memcpy, for _Complex's conversions
// djinterp
#include "./elementary.h"        // d_math_*_sqrt_c, *_sin_c, *_asin_c, *_nan
#include "./exponential.h"       // d_math_*_exp_c, *_log1p_c, *_infinity
#include "./float_order.h"       // d_internal_math_*_pow2, *_binade_of
#include "./hyperbolic.h"        // d_math_*_sinh_c, *_atanh_c
#include "./math_common.h"       // D_MATH_FN
#include "./nearest_integer.h"   // d_math_*_floor_c
#include "./power.h"             // d_math_*_hypot_c, *_pow_c
#include "./tangent.h"           // d_math_*_atan2_c, *_negative
"""

CPLX_TAIL = r"""

#endif  // DJINTERP_C_MATH_CPLX_H
"""

CPLX_UNARIES = ["exp", "log", "sqrt", "sin", "cos", "tan", "asin", "acos",
                "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh"]

# the five C99 has only for real arguments, of a complex one
CPLX_FIVE_UNARIES = ["exp2", "log2", "log10", "log1p", "expm1"]

_CZ = ("struct dj_cplx_@S@", "_z")
_CW = ("struct dj_cplx_@S@", "_w")
_CX = ("@T@", "_x")
_CY = ("@T@", "_y")
CPLX_OVERLOADS = (
    [("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_make", [_CX, _CY]),
     ("D_MATH_FN", "@T@", "d_cplx_real", [_CZ]),
     ("D_MATH_FN", "@T@", "d_cplx_imag", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_conj", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_proj", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_neg", [_CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_add", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_sub", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_mul", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_div", [_CZ, _CW]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_add_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_sub_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_real_sub", [_CX, _CZ]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_mul_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_div_real", [_CZ, _CX]),
     ("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_real_div", [_CX, _CZ]),
     ("D_MATH_FN", "bool", "d_cplx_equal", [_CZ, _CW]),
     ("D_MATH_FN", "@T@", "d_cplx_abs_c", [_CZ]),
     ("D_MATH_FN_RT", "@T@", "d_cplx_abs", [_CZ]),
     ("D_MATH_FN", "@T@", "d_cplx_arg_c", [_CZ]),
     ("D_MATH_FN_RT", "@T@", "d_cplx_arg", [_CZ])]
    + [entry
       for f in CPLX_UNARIES
       for entry in (("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_" + f + "_c", [_CZ]),
                     ("D_MATH_FN_RT", "struct dj_cplx_@S@", "d_cplx_" + f, [_CZ]))]
    + [("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_pow_c", [_CZ, _CW]),
       ("D_MATH_FN_RT", "struct dj_cplx_@S@", "d_cplx_pow", [_CZ, _CW])]
    + [entry
       for f in CPLX_FIVE_UNARIES
       for entry in (("D_MATH_FN", "struct dj_cplx_@S@", "d_cplx_" + f + "_c", [_CZ]),
                     ("D_MATH_FN_RT", "struct dj_cplx_@S@", "d_cplx_" + f, [_CZ]))]
)


CPLX_POW_GENERAL_PLAIN = r"""// d_internal_cplx_@S@_pow_general@KS@
//   _z^_w = exp(w log z) by the _c kernel's general case (section 3.1),
// rounded once to the family. C99's formula in the family's own type would
// carry the error exp magnifies, |w log z| times log's - hundreds of steps
// at |w log z| near 16 - and no C library function forms w log z any
// closer; each operation is as accurate as the best algorithm for it
// (S266), so the plain pow is fast only where section 3's pow takes a case
// of its own first, on the family's real functions.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_general@KS@(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const struct dj_cplx_ld r = d_internal_cplx_ld_pow_general_c(
        d_internal_cplx_ld_make((long double)_z.part[0],
                                (long double)_z.part[1]),
        d_internal_cplx_ld_make((long double)_w.part[0],
                                (long double)_w.part[1]));

    return d_internal_cplx_@S@_make((@T@)r.part[0],
                                    (@T@)r.part[1]);
}

"""


CPLX_POW_GENERAL_PURE = r"""// d_internal_cplx_@S@_scale_pair@KS@
//   _p 2^_k rounded once, _p a pair: its magnitude as m 2^e, m in [1/sqrt(2),
// sqrt(2)], exactly, then scaled as the exponential scales its own (its
// section 4.1), into the subnormals and past the largest finite value; the
// sign last. A zero pair is a zero of its high part's sign.
@KQ@ @T@
d_internal_cplx_@S@_scale_pair@KS@(
    struct dj_internal_math_ld_pair _p,
    int                             _k
)
{
    const bool        negative = (_p.high < (@T@)0);
    const long double high     = negative ? -_p.high : _p.high;
    const long double low      = negative ? -_p.low : _p.low;

    if (_p.high == (@T@)0)
    {
        return _p.high;
    }

    {
        const struct dj_internal_math_ld_log_arg arg =
            d_internal_math_ld_log_arg_of(high);
        struct dj_internal_math_ld_exp_split split = { 1.0L, 0.0L, 0 };
        long double                          value = 0.0L;

        split.y    = arg.m;
        split.y_lo = d_internal_cplx_@S@_scale(low, -arg.exponent);
        split.k    = _k + arg.exponent;
        value      = d_internal_math_ld_exp_scale(split);

        return negative ? -value : value;
    }
}

// d_internal_cplx_@S@_log_modulus_pair@KS@
//   log |(_x + _x_lo) + i _y| as a pair, both parts finite and one nonzero,
// _x_lo below _x's last place (0 but where x is itself a pair): half the
// logarithm of |z|^2, the larger part scaled into [1, 2) by a power of 2 -
// the squares exactly, their sum and 2 x x_lo as a pair to 2^-2P of it, a
// smaller part that the scaling takes into the subnormals far below that -
// by the logarithm past long double's precision (power.h's section 1.5), the
// square in [1, 8): a normal value.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_log_modulus_pair@KS@(
    @T@ _x,
    @T@ _x_lo,
    @T@ _y
)
{
    const int         e0 = d_internal_cplx_@S@_magnitude_exponent(
                               d_internal_cplx_@S@_make(_x, _y));
    const long double sx    = d_internal_cplx_@S@_scale(_x, -e0);
    const long double sx_lo = d_internal_cplx_@S@_scale(_x_lo, -e0);
    const long double sy    = d_internal_cplx_@S@_scale(_y, -e0);
    const struct dj_internal_math_ld_pair xx =
        d_internal_math_ld_product_exact(sx, sx);
    const struct dj_internal_math_ld_pair yy =
        d_internal_math_ld_product_exact(sy, sy);
    const struct dj_internal_math_ld_pair head =
        d_internal_math_ld_two_sum(xx.high, yy.high);
    const struct dj_internal_math_ld_pair square =
        d_internal_math_ld_two_sum(
            head.high,
            head.low + ((xx.low + yy.low) + ((2.0L * sx) * sx_lo)));
    // log |z|^2 = log(square.high 2^(2 e0)) + square.low / square.high
    const struct dj_internal_math_ld_log_arg sa =
        d_internal_math_ld_log_arg_normal(square.high);
    const struct dj_internal_math_ld_pair twice =
        d_internal_math_ld_pair_sum(
            d_internal_math_ld_log_accurate_of(sa.exponent + (2 * e0),
                                               sa.m),
            d_internal_math_ld_pair_of(square.low / square.high, 0.0L));

    return d_internal_math_ld_pair_of(twice.high / 2.0L, twice.low / 2.0L);
}

// d_internal_cplx_@S@_pow_general@KS@
//   _z^_w = exp(w log z), with w log z carried as pairs, so that the error
// exp magnifies by |w log z| is the pairs' own, some 2^-(2P - 8) of it: log
// |z| as log_modulus_pair's (above); arg z as the arc-tangent's pair
// (tangent.h's section 1.3); each product of w's parts by Dekker's. Then
// e^Re as a pair (power.h's section 1.6), and the cosine and sine of Im,
// its high part reduced exactly (elementary.h's section 1.4) and its low
// part taken by the angle's sum; each part the product of the two, rounded
// once. For a part
// of _z or _w infinite or NaN, for _z = 0 and past |w| = 2^(EMAX/2), where
// Dekker's products could overflow, exp(w log z) itself, Annex G's values.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_general@KS@(
    struct dj_cplx_@S@ _z,
    struct dj_cplx_@S@ _w
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ a   = _w.part[0];
    const @T@ b   = _w.part[1];
    const @T@ big = d_internal_math_@S@_pow2(@EMAX@ / 2);

    // what the pairs do not take: C99's formula, Annex G's values
    if ( (!d_internal_cplx_@S@_finite(_z)) ||
         (!d_internal_cplx_@S@_finite(_w)) ||
         ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         (d_internal_cplx_@S@_abs(a) > big) ||
         (d_internal_cplx_@S@_abs(b) > big) )
    {
        return d_internal_cplx_@S@_exp@KS@(
            d_cplx_@S@_mul(_w, d_internal_cplx_@S@_log@KS@(_z)));
    }

    {
        // log |z|, as a pair
        const struct dj_internal_math_ld_pair lr =
            d_internal_cplx_@S@_log_modulus_pair@KS@(x, (@T@)0, y);
        struct dj_internal_math_ld_pair li = { 0.0L, 0.0L };

        // arg z: on an axis a multiple of pi/2, else the arc-tangent's pair
        if (y == (@T@)0)
        {
            li = (x > (@T@)0)
                     ? d_internal_math_ld_pair_of(y, 0.0L)
                     : d_internal_math_ld_atan_reduced(0.0L, 0.0L, 2, false);
        }
        else if (x == (@T@)0)
        {
            li = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 1, false);
        }
        else
        {
            li = d_internal_math_ld_atan2_of(d_internal_cplx_@S@_abs(y), x);
        }

        if (d_internal_math_@S@_negative(y))
        {
            li.high = -li.high;
            li.low  = -li.low;
        }

        {
            // w log z = (a lr - b li) + i (a li + b lr), in pairs
            const struct dj_internal_math_ld_pair ap =
                d_internal_math_ld_pair_of(a, 0.0L);
            const struct dj_internal_math_ld_pair bp =
                d_internal_math_ld_pair_of(b, 0.0L);
            const struct dj_internal_math_ld_pair bli =
                d_internal_math_ld_pair_product(bp, li);
            const struct dj_internal_math_ld_pair tr =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_product(ap, lr),
                    d_internal_math_ld_pair_of(-bli.high, -bli.low));
            const struct dj_internal_math_ld_pair ti =
                d_internal_math_ld_pair_sum(
                    d_internal_math_ld_pair_product(ap, li),
                    d_internal_math_ld_pair_product(bp, lr));
            // the cosine and sine of ti's high part, by its quadrant
            const struct dj_internal_math_ld_sin_cos sc =
                d_internal_math_ld_sin_cos_of(ti.high);
            struct dj_internal_math_ld_pair c =
                ((sc.quadrant % 2) == 0) ? sc.cosine : sc.sine;
            struct dj_internal_math_ld_pair s =
                ((sc.quadrant % 2) == 0) ? sc.sine : sc.cosine;
            const @T@ inf = d_internal_math_@S@_infinity();

            if ( (sc.quadrant == 1) ||
                 (sc.quadrant == 2) )
            {
                c.high = -c.high;
                c.low  = -c.low;
            }

            if (sc.quadrant >= 2)
            {
                s.high = -s.high;
                s.low  = -s.low;
            }

            // ti's low part: to first order below 2^-40, where its square
            // lies past the pairs' precision; else by the angle's sum
            if (d_internal_math_ld_abs(ti.low) < d_internal_math_ld_pow2(-40))
            {
                const struct dj_internal_math_ld_pair c0 = c;

                c = d_internal_math_ld_two_sum(c.high,
                                               c.low - (ti.low * s.high));
                s = d_internal_math_ld_two_sum(s.high,
                                               s.low + (ti.low * c0.high));
            }
            else
            {
                const struct dj_internal_math_ld_sin_cos lo =
                    d_internal_math_ld_sin_cos_of(ti.low);
                const struct dj_internal_math_ld_pair cl =
                    ((lo.quadrant % 2) == 0) ? lo.cosine : lo.sine;
                const struct dj_internal_math_ld_pair sl =
                    ((lo.quadrant % 2) == 0) ? lo.sine : lo.cosine;
                const long double cs = ( (lo.quadrant == 1) ||
                                         (lo.quadrant == 2) ) ? -1.0L : 1.0L;
                const long double ss = (lo.quadrant >= 2) ? -1.0L : 1.0L;
                const struct dj_internal_math_ld_pair cl2 =
                    d_internal_math_ld_pair_of(cs * cl.high, cs * cl.low);
                const struct dj_internal_math_ld_pair sl2 =
                    d_internal_math_ld_pair_of(ss * sl.high, ss * sl.low);
                const struct dj_internal_math_ld_pair cc =
                    d_internal_math_ld_pair_product(c, cl2);
                const struct dj_internal_math_ld_pair snl =
                    d_internal_math_ld_pair_product(s, sl2);
                const struct dj_internal_math_ld_pair scl =
                    d_internal_math_ld_pair_product(s, cl2);
                const struct dj_internal_math_ld_pair csl =
                    d_internal_math_ld_pair_product(c, sl2);

                c = d_internal_math_ld_pair_sum(
                        cc, d_internal_math_ld_pair_of(-snl.high, -snl.low));
                s = d_internal_math_ld_pair_sum(scl, csl);
            }

            // far past either end of the range: infinities or zeros in the
            // angle's directions
            if (tr.high >= 16000.0L)
            {
                return d_internal_cplx_@S@_make(
                    (c.high < 0.0L) ? -inf : inf,
                    (s.high == 0.0L) ? (@T@)s.high
                                     : ((s.high < 0.0L) ? -inf : inf));
            }

            if (tr.high <= -16000.0L)
            {
                return d_internal_cplx_@S@_make(
                    (c.high < 0.0L) ? (@T@)-0.0L : (@T@)0,
                    (s.high < 0.0L) ? (@T@)-0.0L : (@T@)0);
            }

            {
                // e^tr as (y + y_lo) 2^k, times each of the angle's pairs
                const struct dj_internal_math_ld_exp_split e =
                    d_internal_math_ld_exp_pair_split_of(tr);
                const struct dj_internal_math_ld_pair m =
                    d_internal_math_ld_pair_of(e.y, e.y_lo);

                return d_internal_cplx_@S@_make(
                    d_internal_cplx_@S@_scale_pair@KS@(
                        d_internal_math_ld_pair_product(m, c), e.k),
                    d_internal_cplx_@S@_scale_pair@KS@(
                        d_internal_math_ld_pair_product(m, s), e.k));
            }
        }
    }
}

"""


# the five functions C99 has only for real arguments, of a complex one: their
# pure kernels, once, in long double (section 3.1), which both forms of every
# family round
CPLX_POW_INT_PURE = r"""// dj_internal_cplx_@S@_power@KS@
//   struct: a whole power as it is formed, (re + i im) 2^e, each part a pair,
// high + low, carried to some 2^-2P of the larger part (S370).
struct dj_internal_cplx_@S@_power@KS@
{
    struct dj_internal_math_ld_pair re;
    struct dj_internal_math_ld_pair im;
    long                            e;
};

// d_internal_cplx_@S@_pair_dot@KS@
//   _a _b + _c _d, each operand a pair, as a pair, by power.h's products and
// sums of pairs: to some 2^-2P of the larger term. Where the sum is exactly
// 0, the highs' own formula's zero, so that its sign is the one
// d_cplx_@S@_mul's exact products give (section 2.3).
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_pair_dot@KS@(
    struct dj_internal_math_ld_pair _a,
    struct dj_internal_math_ld_pair _b,
    struct dj_internal_math_ld_pair _c,
    struct dj_internal_math_ld_pair _d
)
{
    struct dj_internal_math_ld_pair s = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_product(_a, _b),
        d_internal_math_ld_pair_product(_c, _d));

    // exactly 0: the formula's zero, with its sign
    if (s.high == (@T@)0)
    {
        const @T@ t = (_a.high * _b.high) + (_c.high * _d.high);

        s.high = (t == (@T@)0) ? t : (@T@)0;
        s.low  = (@T@)0;
    }

    return s;
}

// d_internal_cplx_@S@_pair_negated@KS@
//   -_p, exactly.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_pair_negated@KS@(
    struct dj_internal_math_ld_pair _p
)
{
    return d_internal_math_ld_pair_of(-_p.high, -_p.low);
}

// d_internal_cplx_@S@_pair_over@KS@
//   _u / _n, both pairs, |_u| <= _n and _n in [1, 8), as a pair: the
// magnitudes' quotient by elementary.h's ratio, renormalized, the sign
// _u's. Where _u is exactly 0, _zero, the formula's zero, over _n, as
// d_internal_cplx_@S@_over takes a zero, so that the sign is the division's,
// the formula's (S382).
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_pair_over@KS@(
    struct dj_internal_math_ld_pair _u,
    @T@                             _zero,
    struct dj_internal_math_ld_pair _n
)
{
    // exactly 0: the division's zero, its sign kept
    if (_u.high == (@T@)0)
    {
        const @T@ q = _zero / _n.high;

        return d_internal_math_ld_pair_of(q, (@T@)0);
    }

    {
        const bool negative = (_u.high < (@T@)0);
        const struct dj_internal_math_ld_pair a =
            negative ? d_internal_cplx_@S@_pair_negated@KS@(_u) : _u;
        const struct dj_internal_math_ld_pair r =
            d_internal_math_ld_ratio(a.high, a.low, _n.high, _n.low);
        const struct dj_internal_math_ld_pair q =
            d_internal_math_ld_two_sum(r.high, r.low);

        return negative ? d_internal_cplx_@S@_pair_negated@KS@(q) : q;
    }
}

// d_internal_cplx_@S@_power_times@KS@
//   _m _w, as pairs: (a + bi)(c + di) = (ac - bd) + (ad + bc)i, each part by
// d_internal_cplx_@S@_pair_dot@KS@, its terms in d_cplx_@S@_mul's order;
// _m's exponent kept.
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_power_times@KS@(
    struct dj_internal_cplx_@S@_power@KS@ _m,
    struct dj_internal_cplx_@S@_power@KS@ _w
)
{
    struct dj_internal_cplx_@S@_power@KS@ result = _m;

    result.re = d_internal_cplx_@S@_pair_dot@KS@(
        _m.re, _w.re, d_internal_cplx_@S@_pair_negated@KS@(_m.im), _w.im);
    result.im = d_internal_cplx_@S@_pair_dot@KS@(_m.re, _w.im, _m.im, _w.re);

    return result;
}

// d_internal_cplx_@S@_power_inverse@KS@
//   1 / _m, as pairs, as d_cplx_@S@_div takes 1 over _m: the conjugate over
// the squared modulus n = re^2 + im^2, a pair in [1, 8) where _m's larger
// part lies in [1, 2), so that neither part exceeds it; each part's quotient
// by d_internal_cplx_@S@_pair_over@KS@, its zero the division's. _m's
// exponent kept.
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_power_inverse@KS@(
    struct dj_internal_cplx_@S@_power@KS@ _m
)
{
    const struct dj_internal_math_ld_pair n =
        d_internal_cplx_@S@_pair_dot@KS@(_m.re, _m.re, _m.im, _m.im);
    struct dj_internal_cplx_@S@_power@KS@ result = _m;

    // the numerator (1 + 0i)(re - im i): 1 re + 0 im, and 0 re - 1 im
    result.re = d_internal_cplx_@S@_pair_over@KS@(
        _m.re,
        ((@T@)1 * _m.re.high) + ((@T@)0 * _m.im.high),
        n);
    result.im = d_internal_cplx_@S@_pair_over@KS@(
        d_internal_cplx_@S@_pair_negated@KS@(_m.im),
        ((@T@)0 * _m.re.high) + ((@T@)-1 * _m.im.high),
        n);

    return result;
}

// d_internal_cplx_@S@_power_rescaled@KS@
//   _m with the binade E of its larger part's high taken out of both parts,
// high and low, exactly, and added to its exponent: that part into [1, 2),
// so that the next product neither overflows nor underflows but far below
// it. _m not 0.
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_power_rescaled@KS@(
    struct dj_internal_cplx_@S@_power@KS@ _m
)
{
    const @T@ a = d_internal_cplx_@S@_abs(_m.re.high);
    const @T@ b = d_internal_cplx_@S@_abs(_m.im.high);
    const int k = d_internal_cplx_@S@_exponent((a > b) ? a : b);
    struct dj_internal_cplx_@S@_power@KS@ result = _m;

    result.re = d_internal_math_ld_pair_of(
        d_internal_cplx_@S@_scale(_m.re.high, -k),
        d_internal_cplx_@S@_scale(_m.re.low, -k));
    result.im = d_internal_math_ld_pair_of(
        d_internal_cplx_@S@_scale(_m.im.high, -k),
        d_internal_cplx_@S@_scale(_m.im.low, -k));
    result.e  = _m.e + (long)k;

    return result;
}

// d_internal_cplx_@S@_pow_int_pairs@KS@
//   _z^_n, _z finite and not 0, _n an int not 0, as pairs and a power of 2:
// repeated squaring from the exponent's highest bit down, as the matrix
// power is (S262), each product of pairs and rescaled after it so that no
// step overflows or underflows, the exponent clamped where the result is
// certainly an infinity or 0; for _n < 0, 1 / z^|n| at the end, as pairs.
// Each product is within some 2^-2P of its larger part, so that a part that
// cancels keeps its own precision to within that, and a power exact in twice
// long double's precision comes exactly (S370).
@KQ@ struct dj_internal_cplx_@S@_power@KS@
d_internal_cplx_@S@_pow_int_pairs@KS@(
    struct dj_cplx_@S@ _z,
    int                _n
)
{
    const unsigned           count = (_n < 0) ? (0u - (unsigned)_n)
                                              : (unsigned)_n;
    const long               limit =
        4L * ((long)@EMAX@ + (long)@P@);
    const int                e0    = d_internal_cplx_@S@_magnitude_exponent(_z);
    const struct dj_cplx_@S@ z     = d_internal_cplx_@S@_scaled_by(_z, -e0);
    struct dj_internal_cplx_@S@_power@KS@ base =
        { { (@T@)0, (@T@)0 },
          { (@T@)0, (@T@)0 },
          0L };
    struct dj_internal_cplx_@S@_power@KS@ m =
        { { (@T@)0, (@T@)0 },
          { (@T@)0, (@T@)0 },
          0L };
    unsigned bit = 1u;

    base.re = d_internal_math_ld_pair_of(z.part[0], (@T@)0);
    base.im = d_internal_math_ld_pair_of(z.part[1], (@T@)0);
    base.e  = (long)e0;
    m       = base;

    // the highest bit
    while ((bit <= (count / 2u)) && (bit < 0x40000000u))
    {
        bit = bit * 2u;
    }

    for (bit = bit / 2u; bit > 0u; bit = bit / 2u)
    {
        m   = d_internal_cplx_@S@_power_times@KS@(m, m);
        m.e = 2L * m.e;
        m   = d_internal_cplx_@S@_power_rescaled@KS@(m);

        if ((count & bit) != 0u)
        {
            m   = d_internal_cplx_@S@_power_times@KS@(m, base);
            m.e = m.e + (long)e0;
            m   = d_internal_cplx_@S@_power_rescaled@KS@(m);
        }

        // far past either end: the clamp keeps e in a long
        if (m.e > limit)
        {
            m.e = limit;
        }
        else if (m.e < -limit)
        {
            m.e = -limit;
        }
    }

    if (_n < 0)
    {
        m   = d_internal_cplx_@S@_power_inverse@KS@(m);
        m.e = -m.e;
    }

    return m;
}

// d_internal_cplx_@S@_pow_int@KS@
//   _z^_n, _z finite and not 0, _n an int not 0: the pairs of
// d_internal_cplx_@S@_pow_int_pairs@KS@, each part rounded once
// (d_internal_cplx_@S@_scale_pair@KS@), into the subnormals and past the
// largest finite value.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_int@KS@(
    struct dj_cplx_@S@ _z,
    int                _n
)
{
    const struct dj_internal_cplx_@S@_power@KS@ m =
        d_internal_cplx_@S@_pow_int_pairs@KS@(_z, _n);

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_scale_pair@KS@(m.re, (int)m.e),
        d_internal_cplx_@S@_scale_pair@KS@(m.im, (int)m.e));
}

"""


CPLX_POW_INT_PLAIN = r"""// d_internal_cplx_@S@_pow_int@KS@
//   _z^_n, _z finite and not 0, _n an int not 0: the _c kernel's (section
// 3.1), its repeated squaring carried in pairs, rounded to long double once
// and then to the family, as the _c form's is (S370): no faster in the
// family's own type to the same precision, so the plain form's whole power
// is the _c form's, bit for bit.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_pow_int@KS@(
    struct dj_cplx_@S@ _z,
    int                _n
)
{
    const struct dj_cplx_ld r = d_internal_cplx_ld_pow_int_c(
        d_internal_cplx_ld_make((long double)_z.part[0],
                                (long double)_z.part[1]),
        _n);

    return d_internal_cplx_@S@_make((@T@)r.part[0],
                                    (@T@)r.part[1]);
}

"""


CPLX_FIVE_PURE = r"""
// The five functions C99 has only for real arguments, of a complex one:
// each in pure arithmetic, once, in long double, its parts carried as pairs
// to past twice the value's precision and rounded once, so that the _c and
// the plain forms alike are this kernel rounded to the family (section 4).
// Where a part is infinite or NaN, the value of the function C99 has that
// this one is a form of: clog's for log2, log10 and log1p, cexp's for exp2
// and expm1 (G.6.3.1, G.6.3.2).

// d_internal_cplx_@S@_times_pair@KS@
//   _p _k rounded once, _p and _k pairs: a zero _p its own zero, of its sign.
@KQ@ @T@
d_internal_cplx_@S@_times_pair@KS@(
    struct dj_internal_math_ld_pair _p,
    struct dj_internal_math_ld_pair _k
)
{
    // a zero: its sign kept, as the product's two-sum would not
    if (_p.high == 0.0L)
    {
        return _p.high;
    }

    return d_internal_math_ld_pair_product(_p, _k).high;
}

// d_internal_cplx_@S@_arg_pair@KS@
//   arg(_x + i _y) as a pair, neither part a NaN: an infinite part taken as
// +-1 and a finite one beside it as a zero of its sign, so that the angle is
// atan2's at the infinities (G.6.3.2); on the real axis 0 for a positive x
// or +0 and pi for a negative one or -0, on the imaginary axis pi/2, each as
// its pair holds it; else the arc-tangent's pair (tangent.h's section 1.3);
// y's sign last, a zero's among them.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_arg_pair@KS@(
    @T@ _x,
    @T@ _y
)
{
    const bool        infinite = ( (d_internal_cplx_@S@_is_inf(_x)) ||
                                   (d_internal_cplx_@S@_is_inf(_y)) );
    const long double one_x    = d_internal_cplx_@S@_is_inf(_x) ? 1.0L
                                                                : 0.0L;
    const long double one_y    = d_internal_cplx_@S@_is_inf(_y) ? 1.0L
                                                                : 0.0L;
    const long double x = infinite
                              ? d_internal_cplx_@S@_copysign(one_x, _x)
                              : _x;
    const long double y = infinite
                              ? d_internal_cplx_@S@_copysign(one_y, _y)
                              : _y;
    struct dj_internal_math_ld_pair result = { 0.0L, 0.0L };

    // the real axis: 0 or pi, by x's sign
    if (y == (@T@)0)
    {
        result = d_internal_math_@S@_negative(x)
                     ? d_internal_math_ld_atan_reduced(0.0L, 0.0L, 2, false)
                     : d_internal_math_ld_pair_of(0.0L, 0.0L);
    }
    // the imaginary axis: pi/2
    else if (x == (@T@)0)
    {
        result = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 1, false);
    }
    else
    {
        result = d_internal_math_ld_atan2_of(d_internal_cplx_@S@_abs(y), x);
    }

    // y's sign, a zero's among them
    if (d_internal_math_@S@_negative(y))
    {
        result.high = -result.high;
        result.low  = -result.low;
    }

    return result;
}

// d_internal_cplx_@S@_log_in@KS@
//   log _z / ln 2 for _ten false, log _z / ln 10 for _ten true: log |z| and
// arg z as pairs, each times 1/ln b as a pair - a head of 32 bits, exact as
// written, and the rest, as exponential.h's section 1.4 holds them - and
// rounded once; on an axis, where |z| is |x| or |y|, the real part the real
// function's, so that a power of b gives its exponent exactly. At 0 and an
// infinite part, clog's real part, -inf or +inf, and atan2's angle over ln
// b; at a NaN, clog's NaNs.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log_in@KS@(
    struct dj_cplx_@S@ _z,
    bool               _ten
)
{
    const @T@ x = _z.part[0];
    const @T@ y = _z.part[1];
    // 1/ln b = k_hi + k_lo, k_hi cut to 32 bits: exact as written
    const struct dj_internal_math_ld_pair k = _ten
        ? d_internal_math_ld_pair_of(
              0.434294481878168880939483642578125L,
              2.508294671164527633848008229439700580367e-11L)
        : d_internal_math_ld_pair_of(
              1.4426950407214462757110595703125L,
              1.67517131648865110689392137426645954153e-10L);

    // a NaN part: clog's value, its imaginary part a NaN
    if ( (x != x) ||
         (y != y) )
    {
        return d_internal_cplx_@S@_log@KS@(_z);
    }

    // zero or an infinite part: log |z| is -inf or +inf
    if ( ( (x == (@T@)0) &&
           (y == (@T@)0) ) ||
         (!d_internal_cplx_@S@_finite(_z)) )
    {
        const long double size = d_math_@S@_hypot@R@(x, y);

        return d_internal_cplx_@S@_make(
            d_math_@S@_log@R@(size),
            d_internal_cplx_@S@_times_pair@KS@(
                d_internal_cplx_@S@_arg_pair@KS@(x, y), k));
    }

    // an axis: |z| a part's magnitude, the real function's logarithm of it
    if ( (x == (@T@)0) ||
         (y == (@T@)0) )
    {
        const long double m = d_internal_cplx_@S@_abs((y == 0.0L) ? x
                                                                  : y);

        return d_internal_cplx_@S@_make(
            _ten ? d_math_@S@_log10@R@(m)
                 : d_math_@S@_log2@R@(m),
            d_internal_cplx_@S@_times_pair@KS@(
                d_internal_cplx_@S@_arg_pair@KS@(x, y), k));
    }

    return d_internal_cplx_@S@_make(
        d_internal_cplx_@S@_times_pair@KS@(
            d_internal_cplx_@S@_log_modulus_pair@KS@(x, (@T@)0, y), k),
        d_internal_cplx_@S@_times_pair@KS@(
            d_internal_cplx_@S@_arg_pair@KS@(x, y), k));
}

// d_internal_cplx_@S@_log2@KS@, d_internal_cplx_@S@_log10@KS@
//   log2 _z and log10 _z (above).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log2@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_log_in@KS@(_z, false);
}

@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log10@KS@(
    struct dj_cplx_@S@ _z
)
{
    return d_internal_cplx_@S@_log_in@KS@(_z, true);
}

// d_internal_cplx_@S@_log1p@KS@
//   log(1 + _z), 1 + z never rounded: 1 + x is the pair two-sum gives, and
// its arc-tangent with y the pair's to first order in its low part. log |1 +
// z| is half the logarithm of |1 + z|^2 = 1 + w, w = 2x + x^2 + y^2 summed
// exactly as an expansion (power.h's section 1.1) and rounded to a pair,
// where 1 + w lies in [1/2, 2] - so that the logarithm of a value near 1
// keeps w's own precision, however near w is to 0 - and elsewhere of the
// squares of 1 + x's pair and y, as clog's power takes |z|^2 (above); past
// 2^30 in a part, of 1 + x rounded, which then moves log |1 + z|, past 20,
// by under 2^-(P+1) of it. At an infinite or NaN part, clog's value at 1 +
// z; at z = -1, -inf with y's zero.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_log1p@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ big = (@T@)1073741824.0L;

    // an infinite or NaN part: clog of 1 + z, which keeps it
    if (!d_internal_cplx_@S@_finite(_z))
    {
        return d_internal_cplx_@S@_log@KS@(
            d_internal_cplx_@S@_make(x + (@T@)1, y));
    }

    // -1: the logarithm of a zero
    if ( (x == (@T@)-1) &&
         (y == (@T@)0) )
    {
        return d_internal_cplx_@S@_make(-d_internal_math_@S@_infinity(), y);
    }

    {
        // 1 + x exactly, as a pair
        const struct dj_internal_math_ld_pair u =
            d_internal_math_ld_two_sum(1.0L, x);
        const @T@ ax = d_internal_cplx_@S@_abs(x);
        const @T@ ay = d_internal_cplx_@S@_abs(y);
        struct dj_internal_math_ld_pair half = { 0.0L, 0.0L };
        struct dj_internal_math_ld_pair angle = { 0.0L, 0.0L };

        // far out: log |1 + z| of 1 + x rounded
        if ( (ax >= big) ||
             (ay >= big) )
        {
            half = d_internal_cplx_@S@_log_modulus_pair@KS@(u.high, (@T@)0,
                                                            y);
        }
        else
        {
            // |1 + z|^2 roughly, to choose the way
            const long double rough = (u.high * u.high) + (y * y);

            if ( (rough >= 0.5L) &&
                 (rough <= 2.0L) )
            {
                // w = 2x + x^2 + y^2, every term exact, summed exactly
                const struct dj_internal_math_ld_pair xx =
                    d_internal_math_ld_product_exact(x, x);
                const struct dj_internal_math_ld_pair yy =
                    d_internal_math_ld_product_exact(y, y);
                struct dj_internal_math_ld_expansion w = { { 0.0L }, 0 };
                struct dj_internal_math_ld_pair      one_w = { 1.0L, 0.0L };
                int                                  i     = 0;

                w = d_internal_math_ld_expansion_grow(w, 2.0L * x);
                w = d_internal_math_ld_expansion_grow(w, xx.high);
                w = d_internal_math_ld_expansion_grow(w, xx.low);
                w = d_internal_math_ld_expansion_grow(w, yy.high);
                w = d_internal_math_ld_expansion_grow(w, yy.low);

                // 1 + w, from the least part up: the parts do not overlap,
                // so each pair sum loses only below 2^-2P of the whole
                for (i = 0; i < w.count; ++i)
                {
                    one_w = d_internal_math_ld_pair_sum(
                        one_w, d_internal_math_ld_pair_of(w.part[i], 0.0L));
                }

                {
                    // log(1 + w) = log(high) + low / high, high in [1/2, 2]
                    const struct dj_internal_math_ld_log_arg a =
                        d_internal_math_ld_log_arg_normal(one_w.high);
                    const struct dj_internal_math_ld_pair twice =
                        d_internal_math_ld_pair_sum(
                            d_internal_math_ld_log_accurate_of(a.exponent,
                                                               a.m),
                            d_internal_math_ld_pair_of(
                                one_w.low / one_w.high, 0.0L));

                    half = d_internal_math_ld_pair_of(twice.high / 2.0L,
                                                      twice.low / 2.0L);
                }
            }
            else
            {
                half = d_internal_cplx_@S@_log_modulus_pair@KS@(u.high,
                                                                u.low, y);
            }
        }

        // arg(1 + z): on the real axis pi, on the imaginary pi/2; else the
        // arc-tangent of y over 1 + x's high part, less y u_lo / |1 + z|^2
        if (y == (@T@)0)
        {
            angle = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 2, false);
        }
        else if (u.high == 0.0L)
        {
            angle = d_internal_math_ld_atan_reduced(0.0L, 0.0L, 1, false);
        }
        else
        {
            const long double d = (u.high * u.high) + (ay * ay);

            angle = d_internal_math_ld_atan2_of(ay, u.high);

            // the low part's correction: below the angle's last place, its
            // square below 2^-2P of it; d is finite while |z| < 2^30
            if ( (u.low != 0.0L) &&
                 (ax < big) &&
                 (ay < big) )
            {
                angle = d_internal_math_ld_two_sum(
                    angle.high, angle.low - ((ay * u.low) / d));
            }
        }

        // y's sign, a zero's among them
        if (d_internal_math_@S@_negative(y))
        {
            angle.high = -angle.high;
            angle.low  = -angle.low;
        }

        return d_internal_cplx_@S@_make(half.high, angle.high);
    }
}

// d_internal_cplx_@S@_sin_cos_series@KS@
//   sin _r and cos _r as pairs, |_r| <= pi/4 (a last place past it does no
// harm), in the struct's sine and cosine, its quadrant 0: their Taylor series
// in pairs, z = r^2 - sin r = r (1 + z (-1/6 + z (1/120 + ... + z (1/11! + z
// q)))) and cos r = 1 + z (-1/2 + z (1/24 + ... + z (1/12! + z q'))), each
// coefficient's head cut to 32 bits, exact as written, and the rest, q and q'
// the series' remainders in long double - so that each value is good to
// some 2^-(P+28) of itself, past elementary.h's own, whose terms round in
// long double: where two such values are subtracted, as e^z - 1 subtracts
// them, the difference keeps that precision.
@KQ@ struct dj_internal_math_ld_sin_cos
d_internal_cplx_@S@_sin_cos_series@KS@(
    long double _r
)
{
    // 1/3!, 1/5!, 1/7!, 1/9!, 1/11!: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair s3 = d_internal_math_ld_pair_of(
        0.166666666627861559391021728515625L,
        3.880510727564493815104166666666666666667e-11L);
    const struct dj_internal_math_ld_pair s5 = d_internal_math_ld_pair_of(
        0.00833333333139307796955108642578125L,
        1.940255363782246907552083333333333333333e-12L);
    const struct dj_internal_math_ld_pair s7 = d_internal_math_ld_pair_of(
        0.0001984126984098111279308795928955078125L,
        2.887284767533105517190600198412698412698e-15L);
    const struct dj_internal_math_ld_pair s9 = d_internal_math_ld_pair_of(
        0.0000027557319217663689414621330797672271728515625L,
        6.322201237935988426313618924041694223986e-16L);
    const struct dj_internal_math_ld_pair s11 = d_internal_math_ld_pair_of(
        0.000000025052108380829718470295119914226233959197998046875L,
        4.612000304756988471215484815854110338567e-18L);
    // 1/4!, 1/6!, 1/8!, 1/10!, 1/12!: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair c4 = d_internal_math_ld_pair_of(
        0.04166666665696538984775543212890625L,
        9.701276818911234537760416666666666666667e-12L);
    const struct dj_internal_math_ld_pair c6 = d_internal_math_ld_pair_of(
        0.0013888888888686778955161571502685546875L,
        2.021099337273173862033420138888888888889e-14L);
    const struct dj_internal_math_ld_pair c8 = d_internal_math_ld_pair_of(
        0.0000248015873012263909913599491119384765625L,
        3.609105959416381896488250248015873015873e-16L);
    const struct dj_internal_math_ld_pair c10 = d_internal_math_ld_pair_of(
        0.00000027557319215443243365371017716825008392333984375L,
        8.542647287186301507160882260223334848986e-17L);
    const struct dj_internal_math_ld_pair c12 = d_internal_math_ld_pair_of(
        0.000000002087675698113355959861792143783532083034515380859375L,
        6.734539380592168883366111482198269845941e-19L);
    const struct dj_internal_math_ld_pair r = d_internal_math_ld_pair_of(_r,
                                                                         0.0L);
    // z = r^2, exactly
    const struct dj_internal_math_ld_pair z =
        d_internal_math_ld_product_exact(_r, _r);
    struct dj_internal_math_ld_pair    sine   = { 0.0L, 0.0L };
    struct dj_internal_math_ld_pair    cosine = { 0.0L, 0.0L };
    struct dj_internal_math_ld_sin_cos result =
        { { 0.0L, 0.0L }, { 0.0L, 0.0L }, 0 };
    long double                        q      = 0.0L;
    long double                        qc     = 0.0L;
    int                                k      = 0;

    // the remainders: sin's from -1/13!, cos's from -1/14!, each by its
    // terms' ratios, to their 22nd terms
    for (k = 22; k >= 6; --k)
    {
        q = 1.0L - ((z.high * q) /
                    (long double)(((2 * k) + 2) * ((2 * k) + 3)));
    }

    for (k = 22; k >= 7; --k)
    {
        qc = 1.0L - ((z.high * qc) /
                     (long double)(((2 * k) + 1) * ((2 * k) + 2)));
    }

    q  = -q / 6227020800.0L;
    qc = -qc / 87178291200.0L;

    // sin r = r (1 - z (1/3! - z (1/5! - z (1/7! - z (1/9! - z (1/11! +
    // z q)))))), in pairs
    sine = d_internal_math_ld_pair_sum(
        s11, d_internal_math_ld_pair_product(
                 z, d_internal_math_ld_pair_of(q, 0.0L)));
    sine = d_internal_math_ld_pair_sum(
        s9, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_sum(
        s7, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_sum(
        s5, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_sum(
        s3, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-sine.high, -sine.low)));
    sine = d_internal_math_ld_pair_product(
        z, d_internal_math_ld_pair_of(-sine.high, -sine.low));
    sine = d_internal_math_ld_pair_sum(
        r, d_internal_math_ld_pair_product(r, sine));

    // cos r = 1 - z (1/2 - z (1/4! - z (1/6! - z (1/8! - z (1/10! - z
    // (1/12! + z q')))))), in pairs
    cosine = d_internal_math_ld_pair_sum(
        c12, d_internal_math_ld_pair_product(
                 z, d_internal_math_ld_pair_of(qc, 0.0L)));
    cosine = d_internal_math_ld_pair_sum(
        c10, d_internal_math_ld_pair_product(
                 z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        c8, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        c6, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        c4, d_internal_math_ld_pair_product(
                z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_of(0.5L, 0.0L),
        d_internal_math_ld_pair_product(
            z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));
    cosine = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_of(1.0L, 0.0L),
        d_internal_math_ld_pair_product(
            z, d_internal_math_ld_pair_of(-cosine.high, -cosine.low)));

    result.sine   = sine;
    result.cosine = cosine;

    return result;
}

// d_internal_cplx_@S@_angle@KS@
//   cos and sin of _q pi/2 + _r + _r_lo as pairs, in the struct's cosine and
// sine, its quadrant 0: _r finite, reduced by pi/2 (elementary.h's section
// 1.3) and the remainder's sine and cosine taken by the series above, _q
// quarter turns added to its quadrant, and _r_lo, below 2^-60 of 1, and the
// reduction's own low part, to first order, their squares past the pairs'
// precision.
@KQ@ struct dj_internal_math_ld_sin_cos
d_internal_cplx_@S@_angle@KS@(
    long double _r,
    long double _r_lo,
    int         _q
)
{
    const struct dj_internal_math_ld_reduced reduced =
        d_internal_math_ld_reduce(_r);
    const struct dj_internal_math_ld_sin_cos sc =
        d_internal_cplx_@S@_sin_cos_series@KS@(reduced.r);
    const long double low = reduced.r_lo + _r_lo;
    const int q = (reduced.quadrant + _q) % 4;
    // cos: c, -s, -c, s; sin: s, c, -s, -c, by quadrant
    struct dj_internal_math_ld_pair c = ((q % 2) == 0) ? sc.cosine : sc.sine;
    struct dj_internal_math_ld_pair s = ((q % 2) == 0) ? sc.sine : sc.cosine;
    struct dj_internal_math_ld_sin_cos result =
        { { 0.0L, 0.0L }, { 0.0L, 0.0L }, 0 };

    if ( (q == 1) ||
         (q == 2) )
    {
        c.high = -c.high;
        c.low  = -c.low;
    }

    if (q >= 2)
    {
        s.high = -s.high;
        s.low  = -s.low;
    }

    result.cosine = d_internal_math_ld_two_sum(c.high,
                                               c.low - (low * s.high));
    result.sine   = d_internal_math_ld_two_sum(s.high,
                                               s.low + (low * c.high));

    return result;
}

// d_internal_cplx_@S@_ln2_turns_digit
//   2 ln 2 / pi's digit _index, base 2^24, from the binary point, as
// elementary.h's section 1.2 holds 2/pi's - 704 digits, 16,896 bits, as a
// string literal, three bytes a digit, computed by the generator - so that y
// ln 2 = (y 2 ln 2 / pi) pi/2 is reduced as elementary.h reduces x = (x 2 /
// pi) pi/2. 0 for a negative _index, 2 ln 2 / pi having no integer part.
@KQ@ long double
d_internal_cplx_@S@_ln2_turns_digit(
    int _index
)
{
    // 2 ln 2 / pi's first 16,896 bits, three bytes a digit, highest first
    const char* const bytes =
@LN2_TURNS@;

    if ( (_index < 0) ||
         (_index >= 704) )
    {
        return 0.0L;
    }

    return ( ((long double)(unsigned char)bytes[3 * _index] * 65536.0L) +
             ((long double)(unsigned char)bytes[(3 * _index) + 1] * 256.0L) ) +
           (long double)(unsigned char)bytes[(3 * _index) + 2];
}

// d_internal_cplx_@S@_ln2_reduced@KS@
//   _y ln 2 = n pi/2 + (r + r_lo), |r| <= pi/4, for |_y| >= 1 and finite:
// elementary.h's reduction by Payne and Hanek's method (its section 1.3),
// with 2 ln 2 / pi's digits in 2/pi's place - y's digits, base 2^24, times
// as many of them as y's exponent reaches, in exact columns of 48-bit
// products; the integer part kept only mod 4, the fraction from its first
// nonzero digit to past twice long double's precision, then times pi/2 -
// so that y ln 2 is reduced exactly for every y, as no product y ln 2 in any
// precision could be past 2^P.
@KQ@ struct dj_internal_math_ld_reduced
d_internal_cplx_@S@_ln2_reduced@KS@(
    long double _y
)
{
    const long double radix   = 16777216.0L;
    const long double ay      = (_y < 0.0L) ? -_y : _y;
    const struct dj_internal_math_ld_binade binade =
        d_internal_math_ld_binade_of(ay);
    // 2^(24 a) <= |y| < 2^(24 (a + 1))
    const int         a       = binade.exponent / 24;
    // pi/2 = pio2_hi + pio2_lo, pio2_hi of 31 bits: exact as written
    const long double pio2_hi = 1.570796326734125614166259765625L;
    const long double pio2_lo = 6.077100506506192601475144209858469968755e-11L;
    long double       digits[6]   = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
    long double       columns[21] = { 0.0L };
    long double       v           = ay * d_internal_math_ld_pow2(-24 * a);
    long double       quadrant    = 0.0L;
    bool              negative    = false;
    int               count       = 0;
    int               first       = 1;
    int               i           = 0;
    int               c           = 0;
    struct dj_internal_math_ld_reduced result = { 0.0L, 0.0L, 0 };

    // y's digits, base 2^24, from 2^(24 a) down: six reach 120 bits
    for (i = 0; ( (i < 6) && (v != 0.0L) ); ++i)
    {
        digits[i] = d_internal_math_ld_floor_small(v);
        v         = (v - digits[i]) * radix;
        count     = i + 1;
    }

    // y 2 ln 2 / pi in columns of weight 2^(-24 c): the columns of weight
    // 2^24 and up, multiples of 4, left out; every sum exact
    for (c = 0; c <= 20; ++c)
    {
        long double sum = 0.0L;

        for (i = 0; i < count; ++i)
        {
            sum += digits[i] *
                   d_internal_cplx_@S@_ln2_turns_digit(c + a - 1 - i);
        }

        columns[c] = sum;
    }

    // the carries, from the last column up
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

    // the signs: the fraction's, then y's, -y being (-n) pi/2 - r
    if (negative != (_y < 0.0L))
    {
        result.r    = -result.r;
        result.r_lo = -result.r_lo;
    }

    if ( (_y < 0.0L) &&
         (quadrant != 0.0L) )
    {
        quadrant = 4.0L - quadrant;
    }

    result.quadrant = (int)quadrant;

    return result;
}

// d_internal_cplx_@S@_cis_ln2@KS@
//   cos(_y ln 2) and sin(_y ln 2) as pairs (angle's), _y finite: below 1 in
// magnitude, y ln 2 itself as a pair, ln 2 held as the exponential holds it,
// within pi/4; past it, reduced by pi/2 exactly (ln2_reduced).
@KQ@ struct dj_internal_math_ld_sin_cos
d_internal_cplx_@S@_cis_ln2@KS@(
    long double _y
)
{
    // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact as written
    const long double ln2_hi = 0.69314718036912381649017333984375L;
    const long double ln2_lo = 1.908214929270587816144265680755001344e-10L;

    if ( (_y < 1.0L) &&
         (_y > -1.0L) )
    {
        const struct dj_internal_math_ld_pair p =
            d_internal_math_ld_product_exact(_y, ln2_hi);
        const struct dj_internal_math_ld_pair t =
            d_internal_math_ld_two_sum(p.high, p.low + (_y * ln2_lo));

        return d_internal_cplx_@S@_angle@KS@(t.high, t.low, 0);
    }

    {
        const struct dj_internal_math_ld_reduced r =
            d_internal_cplx_@S@_ln2_reduced@KS@(_y);

        return d_internal_cplx_@S@_angle@KS@(r.r, r.r_lo, r.quadrant);
    }
}

// d_internal_cplx_@S@_exp2@KS@
//   2^_z = 2^x (cos(y ln 2) + i sin(y ln 2)): 2^x as (m + m_lo) 2^k, the
// base-2 exponential's pair (power.h's section 1.6), times each of the
// angle's pairs (cis_ln2), each part rounded once by its scaling (pow's
// scale_pair), into the subnormals and past the largest finite value; a y
// below 2^-100 raised by 2^200 first, sin(y ln 2) being y ln 2 there to
// 2^-200 of it and no subnormal y ln 2 to lose its bits. Past 2^16600 and
// below 2^-16600, infinities and zeros in the angle's directions; where a
// part is infinite or NaN, cexp's values (G.6.3.1) with y ln 2 for y. y is
// not 0: the real axis is each family's real function's (section 4).
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_exp2@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ nan = d_internal_math_@S@_nan();
    const @T@ inf = d_internal_math_@S@_infinity();

    // x a NaN, y not 0: NaNs
    if (x != x)
    {
        return d_internal_cplx_@S@_make(x, x);
    }

    // y infinite or NaN: -inf gives +0 + i0, +inf +inf + iNaN, a finite x
    // NaNs
    if ( (y != y) ||
         (d_internal_cplx_@S@_is_inf(y)) )
    {
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

    {
        const struct dj_internal_math_ld_sin_cos a =
            d_internal_cplx_@S@_cis_ln2@KS@(y);

        // x infinite, y finite and not 0: +0 or +inf in the angle's
        // directions; and past either end of every family's range
        if ( (d_internal_cplx_@S@_is_inf(x)) ||
             (x >= (@T@)16600) ||
             (x <= (@T@)-16600) )
        {
            const bool up = (x > (@T@)0);

            return d_internal_cplx_@S@_make(
                (a.cosine.high < 0.0L) ? (up ? -inf : (@T@)-0.0L)
                                       : (up ? inf : (@T@)0),
                (a.sine.high < 0.0L) ? (up ? -inf : (@T@)-0.0L)
                                     : (up ? inf : (@T@)0));
        }

        {
            const struct dj_internal_math_ld_exp_split e =
                d_internal_math_ld_exp2_pair_split_of(
                    d_internal_math_ld_pair_of(x, 0.0L));
            const struct dj_internal_math_ld_pair m =
                d_internal_math_ld_pair_of(e.y, e.y_lo);
            const @T@ tiny = d_internal_math_@S@_pow2(-100);

            // a tiny y: sin(y ln 2) is y ln 2, cos 1, y raised by 2^200
            if ( (y < tiny) &&
                 (y > -tiny) )
            {
                // ln 2 = ln2_hi + ln2_lo, ln2_hi cut to 32 bits: exact
                const long double ln2_hi = 0.69314718036912381649017333984375L;
                const long double ln2_lo =
                    1.908214929270587816144265680755001344e-10L;
                const long double ys = y * d_internal_math_ld_pow2(200);
                const struct dj_internal_math_ld_pair p =
                    d_internal_math_ld_product_exact(ys, ln2_hi);
                const struct dj_internal_math_ld_pair t =
                    d_internal_math_ld_two_sum(p.high,
                                               p.low + (ys * ln2_lo));

                return d_internal_cplx_@S@_make(
                    d_internal_cplx_@S@_scale_pair@KS@(m, e.k),
                    d_internal_cplx_@S@_scale_pair@KS@(
                        d_internal_math_ld_pair_product(m, t), e.k - 200));
            }

            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_scale_pair@KS@(
                    d_internal_math_ld_pair_product(m, a.cosine), e.k),
                d_internal_cplx_@S@_scale_pair@KS@(
                    d_internal_math_ld_pair_product(m, a.sine), e.k));
        }
    }
}

// d_internal_cplx_@S@_expm1_pair@KS@
//   e^_r - 1 as a pair, |_r| <= ln 2 / 2 (a last place past it does no
// harm): the exponential's series in pairs, as power.h's section 1.6 has it
// but to r^8 - r + r^2 (1/2 + r (1/3! + r (1/4! + ... + r (1/8! + r g)))),
// the coefficients' heads of 32 bits, exact as written, and g = (1/9!)(1 +
// r/10 (1 + r/11 (...))) in long double - without its leading 1, so that the
// pair keeps its precision relative to e^r - 1 itself, some 2^-(P+28) of it.
@KQ@ struct dj_internal_math_ld_pair
d_internal_cplx_@S@_expm1_pair@KS@(
    long double _r
)
{
    // 1/3! ... 1/8!: heads of 32 bits, exact as written
    const struct dj_internal_math_ld_pair c[6] = {
        { 0.166666666627861559391021728515625L,
          3.880510727564493815104166666666666666667e-11L },
        { 0.04166666665696538984775543212890625L,
          9.701276818911234537760416666666666666667e-12L },
        { 0.00833333333139307796955108642578125L,
          1.940255363782246907552083333333333333333e-12L },
        { 0.0013888888888686778955161571502685546875L,
          2.021099337273173862033420138888888888889e-14L },
        { 0.0001984126984098111279308795928955078125L,
          2.887284767533105517190600198412698412698e-15L },
        { 0.0000248015873012263909913599491119384765625L,
          3.609105959416381896488250248015873015873e-16L } };
    const struct dj_internal_math_ld_pair r =
        d_internal_math_ld_pair_of(_r, 0.0L);
    // r^2 as a pair: r r exactly
    const struct dj_internal_math_ld_pair r2 =
        d_internal_math_ld_product_exact(_r, _r);
    struct dj_internal_math_ld_pair series = { 0.0L, 0.0L };
    long double                     g      = 1.0L;
    int                             n      = 0;

    // 9! g = 1 + r/10 (1 + r/11 (...)), from its 28th term in
    for (n = 28; n >= 10; --n)
    {
        g = 1.0L + ((_r / (long double)n) * g);
    }

    // 1/8! + r g, then 1/7! + r (...) down to 1/3!, in pairs
    series = d_internal_math_ld_pair_sum(
        c[5], d_internal_math_ld_pair_product(
                  r, d_internal_math_ld_pair_of(g / 362880.0L, 0.0L)));

    for (n = 4; n >= 0; --n)
    {
        series = d_internal_math_ld_pair_sum(
            c[n], d_internal_math_ld_pair_product(r, series));
    }

    // 1/2 + r (...), then r + r^2 (...)
    series = d_internal_math_ld_pair_sum(
        d_internal_math_ld_pair_of(0.5L, 0.0L),
        d_internal_math_ld_pair_product(r, series));

    return d_internal_math_ld_pair_sum(
        r, d_internal_math_ld_pair_product(r2, series));
}

// d_internal_cplx_@S@_exp_split@KS@
//   e^_x as (y + y_lo) 2^k, for |_x| below 2^14, to some 2^-(P+28) of it:
// k the integer nearest x / ln 2, r = x - k ln 2 with ln 2 in three parts -
// two heads of 32 bits and the rest, so that k times either head is exact
// and r a pair to 2^-(2P - 15) of 1 - and e^r = 1 + (e^r_hi - 1)(1 + r_lo) +
// r_lo by the series above: power.h's exponential of a pair, whose ln 2 has
// two parts and whose series rounds from r^6 on in long double, is good to
// some 2^-(P+18), which a difference near e^x cos y = 1 would lose.
@KQ@ struct dj_internal_math_ld_exp_split
d_internal_cplx_@S@_exp_split@KS@(
    long double _x
)
{
    const long double log2e = 1.442695040888963407359924681001892137L;
    // ln 2 = l1 + l2 + l3, l1 and l2 of 32 bits each: exact as written
    const long double l1 = 0.69314718036912381649017333984375L;
    const long double l2 =
        0.0000000001908214928843070257169500791860627941787242889404296875L;
    const long double l3 = 4.275175589747648888943734018153096518025e-20L;
    const long double t  = _x * log2e;
    const int         k  = (int)((t < 0.0L) ? (t - 0.5L) : (t + 0.5L));
    const long double kk = (long double)k;
    // x - k l1 exact (Sterbenz's lemma), k l2 exact, k l3 by Dekker's
    // product, every error kept
    const struct dj_internal_math_ld_pair a =
        d_internal_math_ld_two_sum(_x - (kk * l1), -(kk * l2));
    const struct dj_internal_math_ld_pair b =
        d_internal_math_ld_product_exact(kk, l3);
    const struct dj_internal_math_ld_pair r =
        d_internal_math_ld_pair_sum(
            a, d_internal_math_ld_pair_of(-b.high, -b.low));
    const struct dj_internal_math_ld_pair e =
        d_internal_cplx_@S@_expm1_pair@KS@(r.high);
    // 1 + e (1 + r_lo) + r_lo: r_lo below 2^-60, its square past the pair
    const struct dj_internal_math_ld_pair tail =
        d_internal_math_ld_pair_sum(
            e, d_internal_math_ld_pair_of(r.low * (1.0L + e.high), 0.0L));
    const struct dj_internal_math_ld_pair one =
        d_internal_math_ld_two_sum(1.0L, tail.high);
    const struct dj_internal_math_ld_pair y =
        d_internal_math_ld_two_sum(one.high, one.low + tail.low);
    struct dj_internal_math_ld_exp_split result = { 1.0L, 0.0L, 0 };

    result.y    = y.high;
    result.y_lo = y.low;
    result.k    = k;

    return result;
}

// d_internal_cplx_@S@_expm1@KS@
//   e^_z - 1, as pairs throughout and rounded once. Within ln 2 / 2 of the
// imaginary axis, Re = (e^x - 1) - 2 e^x sin^2(y/2) and Im = 2 e^x sin(y/2)
// cos(y/2), e^x - 1 by the series above and y/2's sine and cosine pairs, so
// that neither term rounds before their difference, which loses to the
// cancellation near the curve e^x cos y = 1 through 0 only the pairs' own
// precision, some 2^-(P+28) of the terms; on the imaginary axis Re is -2
// sin^2(y/2), its zero's sign kept where it underflows; a y below 2^-8300,
// whose half a subnormal y would round, takes Im = e^x y and Re = e^x - 1,
// the square past every value. Elsewhere Re = e^x cos y - 1 and Im = e^x
// sin y, e^x as (m + m_lo) 2^k (exp_split) and the 1 subtracted as 2^-k
// before the scaling, so that the difference is formed to the pairs'
// precision too; past k = 16000, the 1 lies below every value, and below k
// = -(2P + 8) the value below half of 1's last place; past e^16000 and
// below e^-16000, infinities in the angle's directions and -1. Where a part
// is infinite or NaN, cexp's value (G.6.3.1) less 1.
@KQ@ struct dj_cplx_@S@
d_internal_cplx_@S@_expm1@KS@(
    struct dj_cplx_@S@ _z
)
{
    const @T@ x   = _z.part[0];
    const @T@ y   = _z.part[1];
    const @T@ ax  = d_internal_cplx_@S@_abs(x);
    const @T@ ay  = d_internal_cplx_@S@_abs(y);
    const @T@ inf = d_internal_math_@S@_infinity();

    // an infinite or NaN part: cexp's value, less 1
    if (!d_internal_cplx_@S@_finite(_z))
    {
        const struct dj_cplx_@S@ e = d_internal_cplx_@S@_exp@KS@(_z);

        return d_internal_cplx_@S@_make(e.part[0] - (@T@)1, e.part[1]);
    }

    // near the imaginary axis: the half angle's way
    if (ax <= (@T@)0.346573590279972654708616060729088284L)
    {
        const struct dj_internal_math_ld_pair e1 =
            d_internal_cplx_@S@_expm1_pair@KS@(x);
        const struct dj_internal_math_ld_pair one =
            d_internal_math_ld_two_sum(1.0L, e1.high);
        const struct dj_internal_math_ld_pair ex =
            d_internal_math_ld_two_sum(one.high, one.low + e1.low);

        // a tiny y: e^x y, and e^x - 1 with y^2 past it
        if (ay < d_internal_math_@S@_pow2(-8300))
        {
            return d_internal_cplx_@S@_make(
                (x == (@T@)0) ? (@T@)-0.0L : e1.high,
                d_internal_math_ld_pair_product(
                    ex, d_internal_math_ld_pair_of(y, 0.0L)).high);
        }

        {
            const struct dj_internal_math_ld_sin_cos h =
                d_internal_cplx_@S@_angle@KS@(y / (@T@)2, 0.0L, 0);
            const struct dj_internal_math_ld_pair es =
                d_internal_math_ld_pair_product(ex, h.sine);
            // 2 e^x sin^2(y/2) and 2 e^x sin(y/2) cos(y/2), doubled exactly
            const struct dj_internal_math_ld_pair t =
                d_internal_math_ld_pair_product(es, h.sine);
            const struct dj_internal_math_ld_pair s =
                d_internal_math_ld_pair_product(es, h.cosine);
            const struct dj_internal_math_ld_pair re =
                d_internal_math_ld_pair_sum(
                    e1, d_internal_math_ld_pair_of(-2.0L * t.high,
                                                   -2.0L * t.low));

            // x = 0: e^iy - 1, its real part -2 sin^2(y/2), a zero's sign
            // kept where it underflows
            return d_internal_cplx_@S@_make(
                (x == (@T@)0) ? (-2.0L * t.high) : re.high,
                2.0L * s.high);
        }
    }

    {
        const struct dj_internal_math_ld_sin_cos a =
            d_internal_cplx_@S@_angle@KS@(y, 0.0L, 0);

        // far past the range: infinities in the angle's directions, or -1
        if (x >= (@T@)16000)
        {
            return d_internal_cplx_@S@_make(
                (a.cosine.high < 0.0L) ? -inf : inf,
                (a.sine.high < 0.0L) ? -inf : inf);
        }

        if (x <= (@T@)-16000)
        {
            return d_internal_cplx_@S@_make(
                -1.0L, (a.sine.high < 0.0L) ? -0.0L : 0.0L);
        }

        {
            const struct dj_internal_math_ld_exp_split e =
                d_internal_cplx_@S@_exp_split@KS@(x);
            const struct dj_internal_math_ld_pair m =
                d_internal_math_ld_pair_of(e.y, e.y_lo);
            const struct dj_internal_math_ld_pair mc =
                d_internal_math_ld_pair_product(m, a.cosine);
            const @T@ im = d_internal_cplx_@S@_scale_pair@KS@(
                d_internal_math_ld_pair_product(m, a.sine), e.k);

            // e^x cos y below half of 1's last place
            if (e.k < -((2 * LDBL_MANT_DIG) + 8))
            {
                return d_internal_cplx_@S@_make((@T@)-1, im);
            }

            // the 1 below every value
            if (e.k > 16000)
            {
                return d_internal_cplx_@S@_make(
                    d_internal_cplx_@S@_scale_pair@KS@(mc, e.k), im);
            }

            return d_internal_cplx_@S@_make(
                d_internal_cplx_@S@_scale_pair@KS@(
                    d_internal_math_ld_pair_sum(
                        mc,
                        d_internal_math_ld_pair_of(
                            -d_internal_math_ld_pow2(-e.k), 0.0L)),
                    e.k),
                im);
        }
    }
}
"""


# 2 ln 2 / pi to 704 digits of 24 bits, exp2's reduction's table: y ln 2 is
# reduced by pi/2 as elementary.h reduces x, with 2 ln 2 / pi's digits in
# 2/pi's place, computed here as 2/pi's are
def ln2_turns_bits(count):
    """floor(2 ln 2 / pi 2^count), from pi by Machin's formula and ln 2 =
    2 atanh(1/3), each series summed in integers with 64 guard bits, to count
    + 64 bits; the floor is taken at both ends of their errors, which must
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

    def atanh_inv(n):
        total = term = one // n
        k = 1
        while term:
            term //= n * n
            k += 2
            total += term // k
        return total

    pi = (16 * atan_inv(5) - 4 * atan_inv(239)) >> guard
    ln2 = (2 * atanh_inv(3)) >> guard
    low = (2 * (ln2 - 2) << count) // (pi + 2)
    high = (2 * (ln2 + 2) << count) // (pi - 2)
    assert low == high, "2 ln 2 / pi: a run of equal bits at the cut"
    return low


def ln2_turns_literal():
    """the table as a C string literal, three bytes a digit, 16 bytes a
    line, highest first."""
    data = ln2_turns_bits(24 * TWO_OVER_PI_DIGITS).to_bytes(
        3 * TWO_OVER_PI_DIGITS, "big")
    rows = []
    for i in range(0, len(data), 16):
        rows.append('        "' +
                    "".join("\\x%02X" % b for b in data[i:i + 16]) + '"')
    return "\n".join(rows)


# the real functions the pure kernels call, by their arity
CPLX_REAL_BINARY = ("atan2", "hypot", "pow")

CPLX_REAL_SHARED = r"""
// The real functions the kernels call, each once: a wrapper of the core's
// _c function, shared, so that a translation unit compiles each real kernel
// once rather than into every complex kernel that calls it (math_common.h's
// D_MATH_FN_KERNEL). The value is the core's own, bit for bit.

"""


def cplx_real_wrappers(text):
    """text with every call of a long double _c real function routed to a
    shared wrapper, and the wrappers, in the order of their names."""
    import re
    names = sorted(set(re.findall(r"\bd_math_ld_([a-z0-9]+)_c\(", text)))
    text = re.sub(r"\bd_math_ld_([a-z0-9]+)_c\(",
                  r"d_internal_cplx_ld_real_\1_c(", text)
    out = [CPLX_REAL_SHARED]
    for name in names:
        out.append("// d_internal_cplx_ld_real_%s_c\n" % name)
        out.append("//   d_math_ld_%s_c, shared.\n" % name)
        out.append("D_MATH_FN_KERNEL long double\n")
        out.append("d_internal_cplx_ld_real_%s_c(\n" % name)
        if name in CPLX_REAL_BINARY:
            out.append("    long double _a,\n    long double _b\n)\n{\n")
            out.append("    return d_math_ld_%s_c(_a, _b);\n}\n\n" % name)
        else:
            out.append("    long double _x\n)\n{\n")
            out.append("    return d_math_ld_%s_c(_x);\n}\n\n" % name)
    return "".join(out), text


# the families a target may evaluate wider than their type, and the test of
# it (math_common.h's section 4)
CPLX_WIDE = {"f": "D_MATH_FLT_EVAL_WIDE", "d": "D_MATH_DBL_EVAL_WIDE"}


def cplx_wide(text, suffix, keep):
    """text with each block "#if @WIDE@ ... [#else  // @WIDE@ ...] #endif  //
    @WIDE@" made the family's test of its width where keep and the family is
    float or double, which a 32-bit x87 evaluates in long double; for long
    double, and for the pure kernels, which are long double's, the block's
    first part left out and its #else part kept."""
    macro = CPLX_WIDE.get(suffix) if keep else None
    out = []
    state = None
    for line in text.split("\n"):
        if line == "#if @WIDE@":
            state = "wide"
            if macro:
                out.append("#if " + macro)
            continue
        if line == "#else  // @WIDE@":
            state = "else"
            if macro:
                out.append("#else  // " + macro)
            continue
        if line == "#endif  // @WIDE@":
            state = None
            if macro:
                out.append("#endif  // " + macro)
            continue
        if (state == "wide") and (not macro):
            continue
        out.append(line)
    return "\n".join(out)


def cplx_kernels(suffix, ctype, prefix, prose, section, sub, title, pure):
    """the kernel text in one working type, its names suffixed _c and on the
    _c real functions where pure, plain on the C library's otherwise. The
    pure kernels are D_MATH_FN_KERNEL, shared by the three families' _c
    forms, and call the real functions through shared wrappers."""
    body = CPLX_KERNELS + CPLX_INVERSE + CPLX_COMPOSED
    body = body.replace("@POW_GENERAL@", CPLX_POW_GENERAL_PURE if pure
                        else CPLX_POW_GENERAL_PLAIN)
    body = body.replace("@POW_INT@", CPLX_POW_INT_PURE if pure
                        else CPLX_POW_INT_PLAIN)
    if pure:
        body += CPLX_FIVE_PURE.replace("@LN2_TURNS@", ln2_turns_literal())
    body = body.replace("@TITLE@", title)
    body = body.replace("@SECTION@", str(section)).replace("@SUB@", str(sub))
    body = body.replace("@KS@", "_c" if pure else "")
    body = body.replace("@R@", "_c" if pure else "")
    body = body.replace("@KQ@", "D_MATH_FN_KERNEL" if pure else "D_MATH_FN_RT")
    text = cplx_wide(instantiate(body, suffix, ctype, prefix, prose), suffix,
                     not pure)
    if pure:
        head, rest = text.split("//" + "-" * 78 + "\n", 1)
        wrappers, rest = cplx_real_wrappers(rest)
        text = head + "//" + "-" * 78 + "\n" + wrappers + rest
    return text


def cplx():
    lines = [
        "Complex numbers: the C core's complex type and C99's complex",
        "functions, for float, double and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  A complex number is struct dj_cplx_f, _d or _ld, one member, part[2],",
        "the real part first: the layout of T[2], which C99's _Complex T and",
        "C++'s std::complex<T> share, so a value converts to either by a copy",
        "(S267). No _Complex is needed, as C11 makes it optional, MSVC has none",
        "and C++ no such type; where the C compiler has it, section 5 converts.",
        "The arithmetic is pure and comes once: a product's parts by exact",
        "products, a quotient by the conjugate over the squared modulus with",
        "its residual's step, the operands scaled by powers of 2 so that",
        "nothing overflows on the way, and Annex G's infinities where an",
        "operand is infinite; an operation with a real operand takes it as",
        "itself (G.5). Where a target evaluates float and double wider than",
        "their types, as a 32-bit x87 does, their exact steps are taken in long",
        "double, which it rounds in its own type.",
        "",
        "  C99's 22 functions: real, imag, conj and proj, exact, once, with the",
        "arithmetic; the other 18 twice, as every real function in the core: the",
        "_c form a constant expression from C++14, one long double kernel of",
        "pure arithmetic on the core's _c real functions, rounded once to the",
        "family, each kernel shared by the three families (D_MATH_FN_KERNEL);",
        "the plain form the same algorithms in the family's own type on the C",
        "library's real functions, fast. The algorithms are the published ones",
        "(S268): Kahan's for the square root, the tangents and the cuts; Hull,",
        "Fairgrieve and Tang's for the inverse functions, as FreeBSD's catrig.c",
        "writes them; log near the unit circle by log1p of the exact |z|^2 - 1;",
        "pow's exp(w log z) with w log z carried as pairs, in both forms. The",
        "branch cuts and the values at the zeros, infinities and NaNs are C99's",
        "Annex G's (G.6), cacosh's and ctanh's as C17 has them (its DR 471); no",
        "floating-point exception is promised, as a constant expression cannot",
        "raise one.",
        "",
        "  And the five C99 has only for real arguments, of a complex number:",
        "exp2, log2, log10, log1p and expm1 (S342). Each is one long double",
        "kernel of pure arithmetic, its parts carried as pairs to past twice the",
        "value's precision and rounded once: log |z| and arg z as power.h's and",
        "tangent.h's pairs, times 1/ln b as a pair; log |1 + z| of 1 + w, w = 2x",
        "+ x^2 + y^2 summed exactly, and 1 + z's angle with 1 + x as a pair;",
        "e^x - 1 and e^x by series in pairs, and sin and cos by their own, so",
        "that expm1's real part, (e^x - 1) - 2 e^x sin^2(y/2), cancels near the",
        "curve e^x cos y = 1 only to some 2^-(P+28) of its terms; and y ln 2",
        "reduced by pi/2 exactly for every y, by 2 ln 2 / pi's digits as",
        "elementary.h reduces x by 2/pi's (S344). Both forms are the kernel",
        "rounded to the family, as standard C's library has none of them to be",
        "faster with, C99 reserving their names and glibc and the UCRT giving",
        "clog10 alone (S266); on the real axis, where the value is real, each",
        "form is the family's real function in that form, and y's zero (S343).",
        "Where a part is infinite or NaN, the value of the function each is a",
        "form of: clog's for log2 and log10, the angle over ln b, and for log1p",
        "at 1 + z; cexp's for exp2, y ln 2 for y, and for expm1, less 1.",
        "",
        "  The bounds, in steps (float_order.h) of each part from the part",
        "correctly rounded, as tests/djinterp/c/modules/math/t_cplx.c checks them at",
        "the points of math_tests_cplx_refs.h and, for the five,",
        "math_tests_cplx_five_refs.h, computed by mpmath: points exact in",
        "float, each family's at the ends of its range, and double's and long",
        "double's at their full width. The product, the quotient, the",
        "modulus and the argument within 1, the product and the quotient",
        "measured 0; the _c forms within 1 for float and double, where long",
        "double carries 11 bits past double's, and for long double within 4 for",
        "tan and tanh, 3 for exp, sin, cos, acos, cosh and asinh, and 2 for the",
        "rest, pow's among them (each measured one less, or better, but asin's",
        "and acosh's, measured 2); the plain forms within 4, 3 for log and",
        "sqrt, 2 for the modulus and the argument and 5 for tan and tanh (6 for",
        "long double's), the C library's own errors among them; the plain",
        "pow's as the _c form's, its general case being the _c kernel's",
        "(S266): C99's exp(w log z) in the family's type would carry |w log z|",
        "times log's error. The five within 1 in both forms and every family,",
        "measured 0: the kernel rounded to long double and then to the family,",
        "a float or double part a step off where the long double one lies on",
        "the family's midpoint. A whole power, z^n for n from -2^30 to 2^30, is",
        "repeated squaring carried in long double's pairs, each part rounded",
        "once, in both forms (S370): exact where the power is exact in twice",
        "long double's precision; each part within a step of the part correctly",
        "rounded where it is at least |n| 2^(4 - P) of the larger part, P long",
        "double's precision, as t_cplx.c checks it at the points of",
        "math_tests_cplx_pow_refs.h, where a part cancels (measured 0 in every",
        "family); and a part below that, which may cancel past what the pairs",
        "carry, within |n| 2^(3 - 2P) of the larger part (measured 0.031 of it).",
        "The plain form's whole power is the _c form's, bit for bit.",
    ]
    text = banner("cplx.h", lines, "2026.10.27", "2026.11.21")
    text += "\n" + CPLX_HEAD
    structs = []
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        structs.append(instantiate(CPLX_STRUCT.replace("@N@", str(n)),
                                   suffix, ctype, prefix, prose))
    text += CPLX_TYPES.replace("@STRUCTS@", "\n".join(structs))
    text += "\n\n//" + "=" * 78 + "\n// 2.  THE ARITHMETIC\n//" + "=" * 78 + "\n"
    text += ("// Pure, once per family: the parts as given, a product's and a quotient's\n"
             "// parts by exact products, and Annex G's infinities (G.5).\n\n")
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        body = (CPLX_ARITH + CPLX_TURNS).replace("@SECTION@", "2").replace("@SUB@", str(n))
        text += cplx_wide(instantiate(body, suffix, ctype, prefix, prose),
                          suffix, True)
    text += "\n\n//" + "=" * 78 + "\n// 3.  THE KERNELS\n//" + "=" * 78 + "\n"
    text += ("// C99's functions, once in pure arithmetic in long double, the _c forms'\n"
             "// kernel, and once in each family's type on the C library's real\n"
             "// functions, the plain forms': one text, the same algorithms.\n\n")
    ld = FAMILIES[2]
    text += cplx_kernels(*ld, 3, 1, "Pure arithmetic, in long double", True)
    for n, fam in enumerate(FAMILIES, 2):
        text += cplx_kernels(*fam, 3, n, "On the C library, in " + fam[3], False)
    text += "\n\n//" + "=" * 78 + "\n// 4.  THE FUNCTIONS\n//" + "=" * 78 + "\n"
    text += "// Each family's C99 functions, in both forms.\n\n"
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        body = CPLX_FUNCTIONS_HEAD.replace("@SECTION@", "4").replace("@SUB@", str(n))
        item = 2
        for f in CPLX_UNARIES:
            doc = CPLX_UNARY_DOC[f][0]
            body += CPLX_UNARY.replace("@F@", f).replace("@DOC@", doc)
        body += CPLX_POW
        body += ("\n// The five C99 has only for real arguments, of a complex "
                 "one: exp2, log2,\n// log10, log1p and expm1.\n")
        for f in CPLX_FIVE_UNARIES:
            doc, where = CPLX_FIVE_DOC[f]
            if where == "positive":
                axis = ("( (_z.part[1] == (@T@)0) &&\n"
                        "         (_z.part[0] > (@T@)0) )")
                where_text = ", x positive"
            elif where == "past -1":
                axis = ("( (_z.part[1] == (@T@)0) &&\n"
                        "         (_z.part[0] > (@T@)-1) )")
                where_text = ", x past -1"
            else:
                axis = "(_z.part[1] == (@T@)0)"
                where_text = ""
            body += (CPLX_FIVE.replace("@F@", f).replace("@DOC@", doc)
                     .replace("@AXIS@", axis).replace("@WHERE@", where_text))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += "\n\n//" + "=" * 78 + "\n// 5.  C99'S _Complex (C ONLY)\n//" + "=" * 78 + "\n"
    text += ("// Where the C compiler has C99's complex types - C, from C99, but for\n"
             "// __STDC_NO_COMPLEX__ and MSVC - each family's converts to and from its\n"
             "// struct by a copy. <complex.h> is not included: its I and complex are\n"
             "// the user's to take, and _Complex is a keyword.\n\n")
    text += ("#if ( (!defined(__cplusplus))          &&                                     \\\n"
             "      (!defined(__STDC_NO_COMPLEX__)) &&                                     \\\n"
             "      (!defined(D_ENV_COMPILER_MSVC)) )\n")
    for n, (suffix, ctype, prefix, prose) in enumerate(FAMILIES, 1):
        body = CPLX_COMPLEX.replace("@SECTION@", "5").replace("@SUB@", str(n))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += "\n#endif  // C99's _Complex\n"
    text += overload_section("6.  ONE NAME FOR EVERY FAMILY (C++)", CPLX_OVERLOADS)
    text += CPLX_TAIL
    return text
