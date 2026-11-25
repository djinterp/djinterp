"""gen_c_math.self_test - the text of self_test.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .common import (banner)
from .exp_d import (EXP_D_P1, EXP_D_P2, EXP_D_SECTION, EXP_D_SECTION_3,
                    exp_d_constants, exp_d_digits_literal, exp_d_hex_literal,
                    exp_d_phase3_digits, exp_d_table_macro)


# -----------------------------------------------------------------------------
# self_test.h: the double family's start-up self-test (chunk 51, S419)
# -----------------------------------------------------------------------------

SELF_TEST_HEAD = r"""
/*
TABLE OF CONTENTS
=================
1.  THE BUILD
    ---------
    1.  Whether formulas fuse
    2.  The self-test
*/

#ifndef DJINTERP_C_MATH_SELF_TEST_H
#define DJINTERP_C_MATH_SELF_TEST_H 1

// djinterp
#include "./dd.h"            // d_math_dd_two_product, d_math_dd_two_sum
#include "./exponential.h"   // d_math_d_exp_c, d_math_d_log_c
#include "./math_common.h"   // D_MATH_FN_RT
"""

SELF_TEST_BODY = r"""

//==============================================================================
// 1.  THE BUILD
//==============================================================================
// The double family's contract (S415) is the same bits on every platform:
// each of its functions correctly rounded, in double arithmetic alone, its
// exact sums and products exact whether or not the compiler fuses a
// multiply and an add. What a header cannot see is the build: flags that
// break IEEE arithmetic (-ffast-math), a compiler that fuses where it should
// not, a target that rounds otherwise. So a program that depends on the same
// bits calls d_math_self_test once, at start-up, in each translation unit
// whose build may differ, and stops where it is not 0.


// 1.1    Whether formulas fuse
//------------------------------------------------------------------------------

// d_math_formulas_fuse
//   whether this translation unit's build fuses a multiply and an add written
// as two statements, as the math headers' plain formulas are - the _fast
// forms among them: then their bits follow the build, and may differ from an
// ISO build's, while the correctly rounded functions' and the exact sums' and
// products' do not. gcc fuses so in its GNU modes where the target has the
// instruction; clang, told not to by the headers' pragma, only under
// -ffp-contract=fast (math_common.h, section 5).
D_MATH_FN_RT bool
d_math_formulas_fuse(void)
{
    // (1 + 2^-30)(1 - 2^-30) is 1 - 2^-60, which rounds to 1: less 1, 0, or,
    // fused, -2^-60. volatile, so that nothing is known before run time
    volatile double a = 1.000000000931322574615478515625;
    volatile double b = 0.999999999068677425384521484375;
    const double    p = a * b;
    const double    d = p - 1.0;

    return d != 0.0;
}


// 1.2    The self-test
//------------------------------------------------------------------------------

// D_MATH_SELF_TEST_PRODUCTS
//   constant: d_math_self_test's bit for an exact product or sum that was not.
#define D_MATH_SELF_TEST_PRODUCTS   1u

// D_MATH_SELF_TEST_EXP
//   constant: d_math_self_test's bit for an exponential not correctly rounded.
#define D_MATH_SELF_TEST_EXP        2u

// D_MATH_SELF_TEST_LOG
//   constant: d_math_self_test's bit for a logarithm not correctly rounded.
#define D_MATH_SELF_TEST_LOG        4u

// d_math_self_test
//   0 where the double family's guarantees hold in this translation unit's
// build, as far as known results show; otherwise D_MATH_SELF_TEST_PRODUCTS,
// D_MATH_SELF_TEST_EXP, D_MATH_SELF_TEST_LOG or more than one: exact products
// and a sum against their known parts, and the exponential and the logarithm
// at arguments each of their three phases decides, the exponential's hardest
// known among them, against their correctly rounded values. Its arguments
// are volatile, so that each is computed at run time. Where double is
// evaluated wider than double, a 32-bit x87, it refuses to compile, as the
// functions it tests do.
D_MATH_FN_RT unsigned
d_math_self_test(void)
{
    // a b = high + low, each exact as written
    static const double products[3][4] = {
@PRODUCTS@
    };
    // x and e^x, correctly rounded
    static const double exps[4][2] = {
@EXPS@
    };
    // x and log(x), correctly rounded
    static const double logs[4][2] = {
@LOGS@
    };
    unsigned result = 0u;
    int      i      = 0;

    for (i = 0; i < 3; ++i)
    {
        volatile double         a = products[i][0];
        volatile double         b = products[i][1];
        const struct dj_math_dd p = d_math_dd_two_product(a, b);
        // the product's parts again, summed: they come back as they are
        const struct dj_math_dd s = d_math_dd_two_sum(products[i][3],
                                                      products[i][2]);

        if ( (p.high != products[i][2]) || (p.low != products[i][3]) ||
             (s.high != products[i][2]) || (s.low != products[i][3]) )
        {
            result |= D_MATH_SELF_TEST_PRODUCTS;
        }
    }

    for (i = 0; i < 4; ++i)
    {
        volatile double x = exps[i][0];

        if (d_math_d_exp_c(x) != exps[i][1])
        {
            result |= D_MATH_SELF_TEST_EXP;
        }
    }

    for (i = 0; i < 4; ++i)
    {
        volatile double x = logs[i][0];

        if (d_math_d_log_c(x) != logs[i][1])
        {
            result |= D_MATH_SELF_TEST_LOG;
        }
    }

    return result;
}
"""

SELF_TEST_TAIL = r"""
#endif  // DJINTERP_C_MATH_SELF_TEST_H
"""

SELF_TEST_PRODUCTS = [
    ("(4503599644147713.0 / 4503599627370496.0)",
     "(4503599644147713.0 / 4503599627370496.0)",
     "(2251799830462465.0 / 2251799813685248.0)",
     "(281475010265089.0 / 20282409603651670423947251286016.0)"),
    ("(3602879701896397.0 / 36028797018963968.0)",
     "(3152519739159347.0 / 4503599627370496.0)",
     "(5044031582654955.0 / 72057594037927936.0)",
     "(1080863910568919.0 / 162259276829213363391578010288128.0)"),
    ("(9007199254740991.0 / 4503599627370496.0)",
     "(4503599627370497.0 / 4503599627370496.0)",
     "2.0",
     "(4503599627370495.0 / 20282409603651670423947251286016.0)"),
]

SELF_TEST_EXPS = [
    ("1.0", "(6121026514868073.0 / 2251799813685248.0)"),
    ("(7293935723874315.0 / 9671406556917033397649408.0)",
     "(562949953845875.0 / 562949953421312.0)"),
    ("(1.0 / 9007199254740992.0)",
     "(4503599627370497.0 / 4503599627370496.0)"),
    ("-0.5", "(2731571253070597.0 / 4503599627370496.0)"),
]

# x and log(x), correctly rounded (chunk 52's references): the first phase
# decides 10, the second 556902.17..., the third 1 + 6 2^-52 and 1 - 2^-52
SELF_TEST_LOGS = [
    ("0x1.4p+3", "0x1.26bb1bbb55516p+1"),
    ("0x1.0fecc59e73f0dp+19", "0x1.a75d58c991751p+3"),
    ("0x1.0000000000006p+0", "0x1.7fffffffffffcp-50"),
    ("0x1.ffffffffffffep-1", "-0x1.0000000000001p-52"),
]


def self_test():
    lines = [
        "The double family's start-up self-test: whether this translation",
        "unit's build keeps the family's guarantees, and whether it fuses the",
        "math headers' plain formulas.",
        "  GENERATED by scripts/gen_c_math_float.py: edit the generator, not",
        "this file.",
        "",
        "  The double family's functions are correctly rounded in double",
        "arithmetic alone, the same bits on every platform (S415). A header",
        "cannot see the build: flags that break IEEE arithmetic, such as",
        "-ffast-math, or a target that rounds otherwise. d_math_self_test",
        "checks exact products and sums, the exponential and the logarithm",
        "against known results, at run time; d_math_formulas_fuse says whether",
        "the build fuses a multiply and an add, which changes the plain",
        "formulas' bits, the _fast forms' among them, but not the correctly",
        "rounded functions'.",
    ]
    text = banner("self_test.h", lines, "2026.11.16", "2026.11.21")
    body = SELF_TEST_BODY
    prods = []
    for i, row in enumerate(SELF_TEST_PRODUCTS):
        prods.append("        { " + ",\n          ".join(row) + " }" +
                     ("," if i + 1 < len(SELF_TEST_PRODUCTS) else ""))
    exps = []
    for i, row in enumerate(SELF_TEST_EXPS):
        exps.append("        { " + ",\n          ".join(row) + " }" +
                    ("," if i + 1 < len(SELF_TEST_EXPS) else ""))
    logs = []
    for i, (x, y) in enumerate(SELF_TEST_LOGS):
        logs.append("        { " + exp_d_hex_literal(x) + ",\n          " +
                    exp_d_hex_literal(y) + " }" +
                    ("," if i + 1 < len(SELF_TEST_LOGS) else ""))
    body = body.replace("@PRODUCTS@", "\n".join(prods))
    body = body.replace("@EXPS@", "\n".join(exps))
    body = body.replace("@LOGS@", "\n".join(logs))
    text += "\n" + SELF_TEST_HEAD + body + SELF_TEST_TAIL
    return text


# the bounds Gappa proves for dd.h's quotients and square root, in u^2
# (chunk 51's record, proofs/dd_div_d.g, dd_div.g, dd_sqrt.g)
DD_BOUNDS = {"div_d": "12", "div": "38", "sqrt": "32"}


def exp_d_section():
    """the double family's section 3.2, its constants filled in."""
    consts, rows = exp_d_constants()
    l240, t224 = exp_d_phase3_digits()
    text = EXP_D_SECTION.replace("@TABLE@", exp_d_table_macro(rows))
    for key in ("INVL", "L1", "L2", "L3"):
        text = text.replace("@" + key + "@", consts[key])
    for i, h in enumerate(EXP_D_P1):
        text = text.replace("@C%d@" % (i + 2), exp_d_hex_literal(h))
    for i, (h, l) in enumerate(EXP_D_P2):
        k = i + 3
        if l is None:
            text = text.replace("@A%d@" % k, exp_d_hex_literal(h))
        else:
            text = text.replace("@A%dH@" % k, exp_d_hex_literal(h))
            text = text.replace("@A%dL@" % k, exp_d_hex_literal(l))
    every = l240 + t224
    third = EXP_D_SECTION_3
    for key, part in (("@DIGITS_A@", every[:3708]), ("@DIGITS_B@", every[3708:])):
        digits = exp_d_digits_literal("", part, 57)
        lines = []
        for i, line in enumerate(digits):
            if i + 1 < len(digits):
                lines.append(line + " " * (79 - len(line)) + "\\")
            else:
                lines.append(line)
        third = third.replace(key, "\n".join(lines))
    return text + third


def exp_d_swap(text):
    """the double family's text with its exponential replaced; expm1_c, which
    takes exp_c's value far above 0, takes the long double kernel's where
    double is evaluated wider, as exp_c refuses there."""
    a = text.index("// 3.2    Exponential\n")
    b = text.index("// 3.3    Natural logarithm\n")
    text = text[:a] + exp_d_section() + "\n" + text[b:]
    old = """    // far above 0: the 1 lies below half of e^x's last place
    if (x > ((long double)(LDBL_MANT_DIG + 2) * 0.6931L))
    {
        return d_math_d_exp_c(_x);
    }
"""
    new = """    // far above 0: the 1 lies below half of e^x's last place; where double
    // is evaluated wider, the long double kernel's e^x, as exp_c refuses
    // there (section 3.2): +inf well past the largest finite result, +inf
    // among them, so that k fits an int, as chunk 50's exp_c had it
    if (x > ((long double)(LDBL_MANT_DIG + 2) * 0.6931L))
    {
#if D_MATH_DBL_EVAL_WIDE
        if (x > ((long double)(DBL_MAX_EXP + 1) * 0.6932L))
        {
            return d_internal_math_d_infinity();
        }

        return d_internal_math_d_exp_scale(d_internal_math_ld_exp_split_of(x));
#else
        return d_math_d_exp_c(_x);
#endif  // D_MATH_DBL_EVAL_WIDE
    }
"""
    assert text.count(old) == 1
    return text.replace(old, new)
