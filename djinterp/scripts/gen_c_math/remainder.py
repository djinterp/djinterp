"""gen_c_math.remainder - the text of remainder.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (FAMILIES, banner, instantiate, overload_section)


# -----------------------------------------------------------------------------
# remainder.h
# -----------------------------------------------------------------------------
# C's remainder functions (7.12.10): fmod, remainder and remquo. Exact -
# the remainder of two values of a family is a value of it - so each
# family's own arithmetic computes it, no kernel in long double and nothing
# rounded.

REMAINDER_TEMPLATE = r"""
//==============================================================================
// @SECTION@.  THE @UPPER@ FAMILY (_@S@)
//==============================================================================


// @SECTION@.1    The truncated remainder
//------------------------------------------------------------------------------
// fmod_c is x - n y, n the quotient x / y truncated toward 0, exactly -
// the remainder of two values of the family is a value of it - in pure
// arithmetic, a constant expression from C++14. m = |y| 2^j, the largest
// not past |x|, is taken away where it fits, then halved, down to |y|: the
// remainder stays below 2m, so every difference is exact (Sterbenz's lemma).
// Twice as many steps as |x|'s binade lies above |y|'s: some 4,200 at most
// for a double, 66,000 for an x87's long double. The result has x's sign, a
// zero's included. The rest is C's (Annex F): an infinite x, or y zero,
// gives NaN; an infinite y gives x; NaN gives NaN. fmod is the C library's,
// exact too: the two agree bit for bit.

D_MATH_FN @T@
d_math_@S@_fmod_c(
    @T@ _x,
    @T@ _y
)
{
    // NaN gives NaN: the argument itself, as in pow_c
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    // an infinite x, or y zero: NaN
    if ( (_x > @MAX@)  ||
         (_x < -@MAX@) ||
         (_y == (@T@)0) )
    {
        return d_internal_math_@S@_nan();
    }

    {
        const @T@ a = (_x < (@T@)0) ? -_x : _x;
        const @T@ b = (_y < (@T@)0) ? -_y : _y;
        @T@ r = a;
        @T@ m = b;
        int j = 0;

        // |x| below |y| - x a zero, or y infinite, among them - is its own
        // remainder
        if (a < b)
        {
            return _x;
        }

        // m = b 2^j, the largest not past a: doubled while twice it is not
        // past a. a - m is exact or rounded, and rounding keeps the order,
        // so it decides as 2m <= a would, and 2m never overflows
        while (m <= (a - m))
        {
            m += m;
            ++j;
        }

        // take m away where it fits, then halve it, j times, down to b: r <
        // 2m throughout. Counted rather than compared with b, which may be
        // subnormal: an x87 takes some 60 times as long over a step that
        // reads one
        for (;;)
        {
            if (r >= m)
            {
                r -= m;
            }

            if (j == 0)
            {
                break;
            }

            m /= (@T@)2;
            --j;
        }

        return (_x < (@T@)0) ? -r : r;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_fmod(
    @T@ _x,
    @T@ _y
)
{
#ifdef __cplusplus
    return std::fmod(_x, _y);
#else
    return @CFMOD@(_x, _y);
#endif
}

// @SECTION@.2    The rounded remainder
//------------------------------------------------------------------------------
// remainder_c is IEEE's remainder, C's remainder: x - n y, n the quotient
// x / y rounded to the nearest integer, a half to the even one - exactly,
// so that it lies within |y| / 2 of zero. In pure arithmetic, a constant
// expression from C++14: fmod_c by 2|y| first, which keeps the quotient's
// parity, then |y| taken away where the remainder is past |y| / 2, and
// again where it is past it still - each difference exact (Sterbenz's
// lemma). A zero result has x's sign. The rest is C's (Annex F): an
// infinite x, or y zero, gives NaN; an infinite y gives x; NaN gives NaN,
// the argument itself. remainder is the C library's, exact too: the two
// agree bit for bit. C++ before C++11 has no remainder, so there it is
// remainder_c.

D_MATH_FN @T@
d_math_@S@_remainder_c(
    @T@ _x,
    @T@ _y
)
{
    // NaN gives NaN: the argument itself
    if (_x != _x)
    {
        return _x;
    }

    if (_y != _y)
    {
        return _y;
    }

    // an infinite x, or y zero: NaN
    if ( (_x > @MAX@)  ||
         (_x < -@MAX@) ||
         (_y == (@T@)0) )
    {
        return d_internal_math_@S@_nan();
    }

    // an infinite y: x, a finite x lying nearer zero than any multiple
    if ( (_y > @MAX@) ||
         (_y < -@MAX@) )
    {
        return _x;
    }

    {
        const @T@ b = (_y < (@T@)0) ? -_y : _y;
        @T@       r = (_x < (@T@)0) ? -_x : _x;

        // |x| below 2b, the quotient's parity kept: where 2b would
        // overflow, |x| is below it already
        if (b <= (@MAX@ / (@T@)2))
        {
            r = d_math_@S@_fmod_c(r, b + b);
        }

        // past b / 2 take b away, and again past it still: a half stays
        // where the quotient is even. Below twice the least normal value
        // b / 2 may not be exact, but 2r is
        if (b < (@MIN@ + @MIN@))
        {
            if ((r + r) > b)
            {
                r -= b;

                if ((r + r) >= b)
                {
                    r -= b;
                }
            }
        }
        else
        {
            const @T@ half = b / (@T@)2;

            if (r > half)
            {
                r -= b;

                if (r >= half)
                {
                    r -= b;
                }
            }
        }

        // x's sign, a zero's included
        return (_x < (@T@)0) ? -r : r;
    }
}

D_MATH_FN_RT @T@
d_math_@S@_remainder(
    @T@ _x,
    @T@ _y
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::remainder(_x, _y);
#elif defined(__cplusplus)
    // C++ before C++11 has no remainder: the pure-arithmetic form serves
    return d_math_@S@_remainder_c(_x, _y);
#else
    return @CREMAINDER@(_x, _y);
#endif
}

// @SECTION@.3    The rounded remainder and its quotient
//------------------------------------------------------------------------------
// remquo_c is C's remquo: remainder_c's result, and through _quo the last
// three bits of the quotient n it rounded - |n| mod 8, 0 to 7, with the
// sign of x / y - what C asks for at least, and what a reduction by a
// quarter turn wants for its quadrant. Pure arithmetic, a constant
// expression from C++14 where the int _quo points to is the evaluation's
// own, as with C++23's std::remquo: fmod_c by 8|y| first, which keeps the
// quotient's last three bits (where 8|y| would overflow, |x| is below it
// already); then 4|y| and 2|y| taken away where they fit, and |y| as
// remainder_c takes it, each difference exact and each counted. The special
// values are remainder_c's, with the quotient 0. A null _quo stores
// nothing. remquo is the C library's: the same remainder, and a quotient
// with the same last three bits - glibc's on x86-64 gives 8 or -8 where
// remquo_c gives 0. C++ before C++11 has no remquo, so there it is
// remquo_c.

D_MATH_FN @T@
d_math_@S@_remquo_c(
    @T@ _x,
    @T@ _y,
    @QP@ _quo
)
{
    @T@ result   = _x;
    @QI@ quotient = 0;

    // NaN gives NaN, the argument itself; an infinite x, or y zero, NaN; an
    // infinite y, x, a finite x lying nearer zero than any multiple
    if (_x != _x)
    {
        result = _x;
    }
    else if (_y != _y)
    {
        result = _y;
    }
    else if ( (_x > @MAX@)  ||
              (_x < -@MAX@) ||
              (_y == (@T@)0) )
    {
        result = d_internal_math_@S@_nan();
    }
    else if ( (_y > @MAX@) ||
              (_y < -@MAX@) )
    {
        result = _x;
    }
    else
    {
        const @T@ b = (_y < (@T@)0) ? -_y : _y;
        @T@       r = (_x < (@T@)0) ? -_x : _x;

        // |x| below 8b, the quotient's last three bits kept: where 8b would
        // overflow, |x| is below it already
        if (b <= (@MAX@ / (@T@)8))
        {
            r = d_math_@S@_fmod_c(r, b * (@T@)8);
        }

        // 4b taken away where it fits, then 2b: below 8b and 4b, each
        // difference is exact. Where 4b or 2b would overflow, r is below it
        if ( (b <= (@MAX@ / (@T@)4)) &&
             (r >= (b * (@T@)4)) )
        {
            r        -= b * (@T@)4;
            quotient += 4;
        }

        if ( (b <= (@MAX@ / (@T@)2)) &&
             (r >= (b + b)) )
        {
            r        -= b + b;
            quotient += 2;
        }

        // r below 2b and the count even: past b / 2 take b away, and again
        // past it still, as remainder_c does - a half stays where the
        // quotient is even. Below twice the least normal value b / 2 may not
        // be exact, but 2r is
        if (b < (@MIN@ + @MIN@))
        {
            if ((r + r) > b)
            {
                r -= b;
                ++quotient;

                if ((r + r) >= b)
                {
                    r -= b;
                    ++quotient;
                }
            }
        }
        else
        {
            const @T@ half = b / (@T@)2;

            if (r > half)
            {
                r -= b;
                ++quotient;

                if (r >= half)
                {
                    r -= b;
                    ++quotient;
                }
            }
        }

        // x's sign, a zero's included; the quotient's last three bits, 8
        // among them as 0, with the sign of x / y
        result   = (_x < (@T@)0) ? -r : r;
        quotient = quotient % 8;

        if ((_x < (@T@)0) != (_y < (@T@)0))
        {
            quotient = -quotient;
        }
    }

    // a null _quo: the remainder alone
    if (_quo != NULL)
    {
        *_quo = quotient;
    }

    return result;
}

D_MATH_FN_RT @T@
d_math_@S@_remquo(
    @T@ _x,
    @T@ _y,
    @QP@ _quo
)
{
    // a null _quo: the remainder alone, as remquo_c gives it - the
    // library's must have somewhere to store
    int  quotient = 0;
    int* where    = (_quo != NULL) ? _quo : &quotient;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return std::remquo(_x, _y, where);
#elif defined(__cplusplus)
    // C++ before C++11 has no remquo: the pure-arithmetic form serves
    return d_math_@S@_remquo_c(_x, _y, where);
#else
    return @CREMQUO@(_x, _y, where);
#endif
}
"""

REMAINDER_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE FLOAT FAMILY (_f)
    ---------------------
    1.  The truncated remainder
    2.  The rounded remainder
    3.  The rounded remainder and its quotient
2.  THE DOUBLE FAMILY (_d)
    ----------------------
    1.  The truncated remainder
    2.  The rounded remainder
    3.  The rounded remainder and its quotient
3.  THE LONG DOUBLE FAMILY (_ld)
    ----------------------------
    1.  The truncated remainder
    2.  The rounded remainder
    3.  The rounded remainder and its quotient
4.  ONE NAME FOR EVERY FAMILY (C++)
    -------------------------------
*/

#ifndef DJINTERP_C_MATH_REMAINDER_H
#define DJINTERP_C_MATH_REMAINDER_H 1

// std
#ifdef __cplusplus
    #include <cmath>         // std::fmod, std::remainder, std::remquo
#endif  // __cplusplus
#include <float.h>           // FLT_MIN ... LDBL_MAX
#include <math.h>            // fmodf ... remquol
#include <stddef.h>          // NULL
// djinterp
#include "./elementary.h"    // d_internal_math_*_nan
#include "./math_common.h"   // D_MATH_FN
"""

REMAINDER_TAIL = r"""

#endif  // DJINTERP_C_MATH_REMAINDER_H
"""

REMAINDER_OVERLOADS = [
    ("D_MATH_FN", "@T@", "d_math_fmod_c", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_remainder_c", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN", "@T@", "d_math_remquo_c", [("@T@", "_x"), ("@T@", "_y"), ("int*", "_quo")]),
    ("D_MATH_FN_RT", "@T@", "d_math_fmod", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN_RT", "@T@", "d_math_remainder", [("@T@", "_x"), ("@T@", "_y")]),
    ("D_MATH_FN_RT", "@T@", "d_math_remquo", [("@T@", "_x"), ("@T@", "_y"), ("int*", "_quo")]),
]


def remainder():
    lines = [
        "Remainders: the C core's fmod, remainder and remquo, for float, double",
        "and long double.",
        "  GENERATED by scripts/gen_c_math_float.py from one text for the three",
        "families: edit the generator, not this file.",
        "",
        "  Each function comes twice, as in elementary.h. The _c form is pure",
        "arithmetic, so the same text is a constant expression from C++14; the",
        "plain form calls the C library (std:: in C++), fast at run time. A",
        "remainder is exact - that of two values of a family is a value of it",
        "- so the _c forms compute it in the family's own arithmetic, by",
        "differences each exact, and agree with the library's bit for bit: a",
        "face may use either and not know which ran. Of remquo's quotient C",
        "promises the last three bits, and those are what agree. C++ before",
        "C++11 has neither remainder nor remquo, so that there they are the _c",
        "forms.",
    ]
    text = banner("remainder.h", lines, "2026.10.10", "2026.11.24")
    text += "\n" + REMAINDER_HEAD
    for i, (suffix, ctype, prefix, prose) in enumerate(FAMILIES):
        upper = prose.upper()
        body = REMAINDER_TEMPLATE.replace("@SECTION@", str(i + 1)).replace("@UPPER@", upper)
        body = body.replace("@CFMOD@", {"float": "fmodf", "double": "fmod", "long double": "fmodl"}[ctype])
        body = body.replace("@CREMAINDER@", {"float": "remainderf", "double": "remainder", "long double": "remainderl"}[ctype])
        body = body.replace("@CREMQUO@", {"float": "remquof", "double": "remquo", "long double": "remquol"}[ctype])
        # remquo's int and int* in line with the family's type
        body = body.replace("@QP@", "int*".ljust(len(ctype))).replace("@QI@", "int".ljust(len(ctype)))
        text += instantiate(body, suffix, ctype, prefix, prose)
    text += overload_section("4.  ONE NAME FOR EVERY FAMILY (C++)", REMAINDER_OVERLOADS)
    text += REMAINDER_TAIL
    return text
