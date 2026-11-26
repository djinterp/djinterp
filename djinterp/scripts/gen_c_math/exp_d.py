"""gen_c_math.exp_d - the text of exp_d.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""


# -----------------------------------------------------------------------------
# exponential.h: the double family's exponential, correctly rounded on doubles
# and pairs of doubles (chunk 51, S416)
# -----------------------------------------------------------------------------
# The constants are computed here with integers alone; the polynomials'
# coefficients are Sollya's (chunk 51's record, proofs/exp_poly.sollya), and
# their error bounds and the three phases' are proven there (proofs/).

EXP_D_BITS = 400


def exp_d_ln2_fixed():
    """ln 2 times 2^EXP_D_BITS, rounded down: the sum of 1/(k 2^k)."""
    one = 1 << EXP_D_BITS
    total, k, term = 0, 1, one // 2
    while term:
        total += term // k
        k += 1
        term //= 2
    return total


def exp_d_pow2_fixed(j):
    """2^(j/128) times 2^EXP_D_BITS, from below: 2's seventh square root by
    repeated integer square roots, then its j-th power."""
    import math
    one = 1 << EXP_D_BITS
    root = 2 * one
    for _ in range(7):
        root = math.isqrt(root << EXP_D_BITS)
    value = one
    for _ in range(j):
        value = (value * root) >> EXP_D_BITS
    return value


def exp_d_round(num, den, bits=53):
    """num / den (den > 0) rounded to bits significant bits, ties to even,
    as (m, e) with value m 2^e, m odd or 0."""
    from fractions import Fraction
    q = Fraction(num, den)
    if q == 0:
        return 0, 0
    sign = -1 if q < 0 else 1
    a = abs(q)
    e = a.numerator.bit_length() - a.denominator.bit_length()
    if Fraction(2) ** e > a:
        e -= 1
    if Fraction(2) ** (e + 1) <= a:
        e += 1
    scale = bits - 1 - e
    m = a * Fraction(2) ** scale
    n = m.numerator // m.denominator
    rest = m - n
    if rest > Fraction(1, 2) or (rest == Fraction(1, 2) and n % 2 == 1):
        n += 1
    while n and n % 2 == 0:
        n //= 2
        scale -= 1
    return sign * n, -scale


def exp_d_literal(m, e):
    """m 2^e, exact as written: an integer, or an integer over a power of 2,
    each a double exactly."""
    if m == 0:
        return "0.0"
    if e >= 0:
        return "%d.0" % (m << e)
    return "(%d.0 / %d.0)" % (m, 1 << -e)


def exp_d_hex_literal(h):
    """a C99 hexadecimal literal's value (Sollya's), as exp_d_literal."""
    from fractions import Fraction
    q = Fraction(float.fromhex(h))
    m, e = exp_d_round(q.numerator, q.denominator)
    assert Fraction(m) * Fraction(2) ** e == q
    return exp_d_literal(m, e)


def exp_d_constants():
    """the reduction's constants and the table, as literals."""
    from fractions import Fraction
    one = 1 << EXP_D_BITS
    ln2 = exp_d_ln2_fixed()
    # 128/ln2 rounded; ln2/128 as L1 + L2 + L3, L1 and L2 of 35 bits
    invl = exp_d_round(128 * one, ln2)
    L = Fraction(ln2, 128 * one)
    m1, e1 = exp_d_round(L.numerator, L.denominator, 35)
    L1 = Fraction(m1) * Fraction(2) ** e1
    m2, e2 = exp_d_round((L - L1).numerator, (L - L1).denominator, 35)
    L2 = Fraction(m2) * Fraction(2) ** e2
    m3, e3 = exp_d_round((L - L1 - L2).numerator, (L - L1 - L2).denominator)
    # exp_fast's second part: ln2/128 - L1 rounded to double (chunk 65)
    mf, ef = exp_d_round((L - L1).numerator, (L - L1).denominator)
    consts = {
        "INVL": exp_d_literal(*invl),
        "L1": exp_d_literal(m1, e1),
        "L2": exp_d_literal(m2, e2),
        "L3": exp_d_literal(m3, e3),
        "L2F": exp_d_literal(mf, ef),
    }
    rows = []
    for j in range(128):
        t = Fraction(exp_d_pow2_fixed(j), one)
        mh, eh = exp_d_round(t.numerator, t.denominator)
        hi = Fraction(mh) * Fraction(2) ** eh
        ml, el = exp_d_round((t - hi).numerator, (t - hi).denominator)
        # hi rounded to a multiple of 2^-25, ties to even, as phase 1's
        # shifter rounds it (S489), and the exact rest
        u = hi * 2 ** 25
        n = u.numerator // u.denominator
        rest = u - n
        if rest > Fraction(1, 2) or (rest == Fraction(1, 2) and n % 2 == 1):
            n += 1
        t1 = Fraction(n, 2 ** 25)
        t2 = hi - t1
        m1t, e1t = exp_d_round(t1.numerator, t1.denominator)
        m2t, e2t = exp_d_round(t2.numerator, t2.denominator)
        assert Fraction(m1t) * Fraction(2) ** e1t == t1
        assert Fraction(m2t) * Fraction(2) ** e2t == t2
        rows.append((exp_d_literal(mh, eh), exp_d_literal(ml, el),
                     exp_d_literal(m1t, e1t), exp_d_literal(m2t, e2t)))
    return consts, rows


def exp_d_phase3_digits():
    """ln2/128 times 2^240 and 2^(j/128) times 2^224, rounded down, as hex
    digit strings of 60 and 57 digits."""
    ln2 = exp_d_ln2_fixed()
    l240 = (ln2 >> 7) >> (EXP_D_BITS - 240)
    t224 = "".join("%057x" % (exp_d_pow2_fixed(j) >> (EXP_D_BITS - 224))
                   for j in range(128))
    return "%060x" % l240, t224


# Sollya's coefficients (proofs/exp_poly.out)
EXP_D_P1 = ["0x1.fffffffffff57p-2", "0x1.55555555555e5p-3",
            "0x1.55555acfaad08p-5", "0x1.1111121e834dp-7"]
EXP_D_P2 = [("0x1.5555555555555p-3", "0x1.5555557ae255ep-57"),
            ("0x1.5555555555555p-5", "0x1.5555c7d4cefc4p-59"),
            ("0x1.1111111111111p-7", None), ("0x1.6c16c16c16bdbp-10", None),
            ("0x1.a01a01a169c75p-13", None), ("0x1.a01a0583a101dp-16", None),
            ("0x1.7171fb5e0981ep-19", None)]


def exp_d_table_macro(rows):
    """the table of 2^(j/128) as a macro's initializer, one value a line, four
    to a row: the pair, then the high part's split."""
    out = ["#define D_INTERNAL_MATH_D_EXP_TABLE" + " " * 44 + "\\",
           "    {" + " " * 74 + "\\"]
    for j, row in enumerate(rows):
        for k, v in enumerate(row):
            last = (j == 127 and k == len(row) - 1)
            text = "        " + v + ("" if last else ",")
            out.append(text + " " * (79 - len(text)) + "\\")
    out.append("    }")
    return "\n".join(out)


def exp_d_digits_literal(name, digits, per=64):
    """a digit string as a C string literal over lines."""
    lines = []
    for i in range(0, len(digits), per):
        lines.append('    "' + digits[i:i + per] + '"')
    return lines

EXP_D_SECTION = r"""// 3.2    Exponential
//------------------------------------------------------------------------------
// exp_c is the exponential correctly rounded, in double arithmetic alone -
// no long double and no C library - and a constant expression from C++14
// (S416). x is reduced to (128 e + j) ln2/128 + r, 128 e + j the integer
// nearest x 128/ln2 and |r| < 0.00271, ln2/128 in three parts whose first two
// products by that integer are exact; then exp(x) = 2^e 2^(j/128) e^r, with
// 2^(j/128) from a table of 128 pairs of doubles, each high part beside its
// split as the first phase takes it. Three phases follow, each
// with a bound proven in chunk 51's record (proofs/), each giving its result
// where the interval its bound allows rounds one way, as Ziv's method has
// it: phase 1, in doubles, within 2^-63.5 of the exponential, relatively,
// answers some 99.8% of arguments; phase 2, in pairs of doubles (dd.h),
// within 2^-101.2; phase 3, in fixed point of 256 bits, within 2^-218.3,
// far closer than a double's exponential is known to come to a rounding
// boundary - the hardest case known, Lefevre and Muller's x =
// 0x1.9e9cbbfd6080bp-31, lies 2^-58.6 of an ulp from one. A result below
// the least normal is rounded to the subnormals at each phase's test. The
// bounds hold whether or not the compiler fuses a multiply and an add
// (math_common.h, section 5). NaN is its own exponential; past the last
// finite result, 709.78, the result is +inf, and below -745.13, +0. Where
// double is evaluated wider than double, a 32-bit x87, exp_c refuses to
// compile: build with -msse2 -mfpmath=sse (math_common.h,
// D_MATH_DBL_REFUSE).
//   exp, the plain form, is exp_c, as the owner's ruling 12 has a plain
// double function once its correctly rounded form is within about twice the
// C library's time (S478): every call is correctly rounded, the same bits on
// every platform and at compile time. Where double is evaluated wider than
// double, exp is the C library's, so that the plain form builds everywhere
// (S479).
//   exp_c's common path, x from 2^-53 to 708 in size and its first phase
// deciding, is inlined into each caller and kept small: the two later phases
// are out of line (math_common.h's D_MATH_FN_COLD), the table holds each
// high part's split, and there the result is normal and below 2^1022, so the
// first phase's rounding needs no test of the subnormals or of overflow
// (S489). The bits are those of the three phases as above, at every x.
//   exp_fast is e^x within 0.511 of an ulp, at the C library's time: x
// reduced with ln2/128 in two parts, e^r - 1 by the first phase's
// polynomial, r + r^2 (c2 + r c3) + r^4 (c4 + r c5), then T (1 + that) from
// the table's pair, rounded once; the bound proven by Gappa whether or not
// the compiler fuses a multiply and an add (chunk 65's record, proofs/).
// Outside 2^-53 < |x| <= 708 it is exp_c, and where double is evaluated
// wider than double, the C library's exp (S490).

@TABLE@

#ifdef __cplusplus
// dj_internal_math_d_exp_table
//   struct: the table as a template's static member, so that every
// translation unit shares one copy and a constant evaluation can read it.
template <int Unused>
struct dj_internal_math_d_exp_table
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr double value[512] = D_INTERNAL_MATH_D_EXP_TABLE;
#else
    static const double value[512];
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
};

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
template <int Unused>
const double dj_internal_math_d_exp_table<Unused>::value[512] =
    D_INTERNAL_MATH_D_EXP_TABLE;
#elif !D_ENV_LANG_IS_CPP17_OR_HIGHER
template <int Unused>
constexpr double dj_internal_math_d_exp_table<Unused>::value[512];
#endif
#endif  // __cplusplus

// d_internal_math_d_exp_t
//   the table's _index-th value: 2^(j/128)'s high part at 4 j, its low part
// at 4 j + 1, the high part rounded to a multiple of 2^-25 at 4 j + 2, and the
// rest of the high part at 4 j + 3, each exact as written. In C a function's
// own table, so that a translation unit that never reaches it holds none.
D_MATH_FN double
d_internal_math_d_exp_t(
    int _index
)
{
#ifdef __cplusplus
    return dj_internal_math_d_exp_table<0>::value[_index];
#else
    static const double table[512] = D_INTERNAL_MATH_D_EXP_TABLE;

    return table[_index];
#endif  // __cplusplus
}

// d_internal_math_d_exp_pow2
//   2^_e for _e from -1022 to 1023, exact: from its bits where the language
// can read a double's - a union in C, gcc's and clang's __builtin_bit_cast
// in C++, a constant expression there - and by float_order.h's squarings
// elsewhere.
D_MATH_FN double
d_internal_math_d_exp_pow2(
    int _e
)
{
#if ( !defined(__cplusplus) && defined(UINT64_MAX) )
    union
    {
        uint64_t bits;
        double   value;
    } v = { 0 };

    v.bits = ((uint64_t)(_e + 1023)) << 52;

    return v.value;
#elif ( defined(__cplusplus) && defined(UINT64_MAX) &&                         \
        defined(D_INTERNAL_MATH_HAS_BIT_CAST) )
    return __builtin_bit_cast(double, ((uint64_t)(_e + 1023)) << 52);
#else
    return d_internal_math_d_pow2(_e);
#endif
}

// dj_internal_math_d_exp_arg
//   struct: x reduced: x = (128 e + j) ln2/128 + r1 + r2, but for less than
// 2^-113.9; |r1| < 0.00271, |r2| < 2^-61.
struct dj_internal_math_d_exp_arg
{
    double k;   // 128 e + j
    int    e;
    int    j;   // 0 to 127
    double r1;
    double r2;
};

// dj_internal_math_d_exp_result
//   struct: a phase's answer: decided, and the correctly rounded value, or
// not decided.
struct dj_internal_math_d_exp_result
{
    bool   decided;
    double value;
};

// d_internal_math_d_exp_arg_of
//   _x, |_x| at most 745.14, reduced. k is the integer nearest t = x 128/ln2,
// t rounded or not (fused), so |k| < 2^18; x - k L1 is exact, a multiple of
// x's ulp below 2^-8 where k is not 0, and x where it is; k L1 and k L2 are
// exact, L1 and L2 of 35 bits; so x - k L1 - k L2 is exactly r1 + r2's first
// form, and k L3 adds below 2^-114. j and e are k's remainder and quotient
// by 128, taken from k + 2^18, which is positive.
D_MATH_FN struct dj_internal_math_d_exp_arg
d_internal_math_d_exp_arg_of(
    double _x
)
{
    // 128/ln2; ln2/128 = L1 + L2 + L3 but for 2^-137
    const double invl  = @INVL@;
    const double l1    = @L1@;
    const double l2    = @L2@;
    const double l3    =
        @L3@;
    // 1.5 2^52: added and taken away, it rounds |t| < 2^51 to an integer
    const double shift = 6755399441055744.0;
    const double t     = _x * invl;
    const double k     = (t + shift) - shift;
    const int    kb    = (int)k + 262144;
    const struct dj_math_dd r = d_math_dd_two_sum(_x - (k * l1), -(k * l2));
    struct dj_internal_math_d_exp_arg result = { 0.0, 0, 0, 0.0, 0.0 };

    result.k  = k;
    result.j  = kb & 127;
    result.e  = (kb >> 7) - 2048;
    result.r1 = r.high;
    result.r2 = r.low - (k * l3);

    return result;
}

// d_internal_math_d_exp_times_pow2
//   _y 2^_e, exact, where the result is normal: _e from -1022 to 1024, 1024
// only where _y < 1.
D_MATH_FN double
d_internal_math_d_exp_times_pow2(
    double _y,
    int    _e
)
{
    return (_e > 1023)
        ? ((_y * 2.0) * d_internal_math_d_exp_pow2(_e - 1))
        : (_y * d_internal_math_d_exp_pow2(_e));
}

// d_internal_math_d_exp_side
//   where _d lies against -1/2 and 1/2: -1 below, 1 above, 0 between, 2 at
// either.
D_MATH_FN int
d_internal_math_d_exp_side(
    double _d
)
{
    if ( (_d == 0.5) || (_d == -0.5) )
    {
        return 2;
    }

    return (_d > 0.5) ? 1 : ((_d < -0.5) ? -1 : 0);
}

// d_internal_math_d_exp_subnormal
//   (_yh + _yl) 2^_e rounded to a multiple of the least subnormal, 2^-1074,
// where that is the rounding - the result below 2^-1021 - and the exact
// value is within _err of _yh + _yl, |_yl| < |_yh|: decided where both ends
// of the interval round alike. The pair is first normalized, exactly, by
// Fast2Sum; z = y 2^(e + 1074), below 2^52, is scaled exactly; n0, the
// integer nearest zh, is exact by the shift, and zh - n0 by Sterbenz's
// lemma.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_exp_subnormal(
    double _yh,
    double _yl,
    double _err,
    int    _e
)
{
    // 2^52
    const double shift = 4503599627370496.0;
    // 2^-1074, the least subnormal, as 2^-1022 2^-52
    const double least = d_internal_math_d_exp_pow2(-1022) *
                         (1.0 / 4503599627370496.0);
    const double scale = d_internal_math_d_exp_pow2(_e + 1074);
    const struct dj_math_dd y = d_math_dd_fast_two_sum(_yh, _yl);
    const double zh    = y.high * scale;
    const double zl    = y.low * scale;
    const double ez    = _err * scale;
    const double n0    = (zh + shift) - shift;
    const double d0    = zh - n0;
    const int    lo    = d_internal_math_d_exp_side(d0 + (zl - ez));
    const int    hi    = d_internal_math_d_exp_side(d0 + (zl + ez));
    struct dj_internal_math_d_exp_result result = { false, 0.0 };

    if ( (lo == hi) && (lo != 2) )
    {
        result.decided = true;
        result.value   = (n0 + (double)lo) * least;
    }

    return result;
}

// d_internal_math_d_exp_decide
//   (_yh + _yl) 2^_e rounded once, where the exact value is within _err of
// _yh + _yl, _yh in [0.99, 2.01]: decided where both ends of the interval
// round alike, in the normal range directly and below it by
// d_internal_math_d_exp_sub.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_exp_decide(
    double _yh,
    double _yl,
    double _err,
    int    _e
)
{
    struct dj_internal_math_d_exp_result result = { false, 0.0 };
    const double low  = _yh + (_yl - _err);
    const double high = _yh + (_yl + _err);

    // below 2^-1022, or within an ulp of it: the subnormals' rounding
    if ( (_e < -1022) || ( (_e == -1022) && (low < 1.0) ) )
    {
        return d_internal_math_d_exp_subnormal(_yh, _yl, _err, _e);
    }

    if (low == high)
    {
        result.decided = true;
        result.value   = d_internal_math_d_exp_times_pow2(low, _e);
    }

    return result;
}

// dj_internal_math_d_exp_estimate
//   struct: the first phase's pair, high + low, and the bound err on its
// distance from exp(x) 2^-e.
struct dj_internal_math_d_exp_estimate
{
    double high;
    double low;
    double err;
};

// d_internal_math_d_exp_estimate_of
//   exp(x) 2^-e from _a in doubles: e^r1 - 1 - r1 = r1^2 q(r1), q of degree 3
// (Sollya's), within 2^-63.66; T r1 split so that its high product, 26 bits
// by 27, is exact, T's split read from the table; T + T r1 taken exactly by
// Fast2Sum. Within 2^-63.5 of exp(x), relatively (proofs/exp_phase1.g): err
// is 2^-63 times the result's high part. Where k is 0, T is 1 and the bound
// shrinks with x^2: 2^-46 z + 2^-52 (|lo| + |b|) (proofs/exp_phase1_n0.g).
D_MATH_FN struct dj_internal_math_d_exp_estimate
d_internal_math_d_exp_estimate_of(
    struct dj_internal_math_d_exp_arg _a
)
{
    const double c2  = @C2@;
    const double c3  = @C3@;
    const double c4  = @C4@;
    const double c5  = @C5@;
    // 1.5 2^17: it rounds r1 to a multiple of 2^-35
    const double s35 = 196608.0;
    const double thi = d_internal_math_d_exp_t(4 * _a.j);
    const double tlo = d_internal_math_d_exp_t((4 * _a.j) + 1);
    const double t1  = d_internal_math_d_exp_t((4 * _a.j) + 2);
    const double t2  = d_internal_math_d_exp_t((4 * _a.j) + 3);
    const double r1  = _a.r1;
    const double z   = r1 * r1;
    const double q   = c2 + (r1 * (c3 + (r1 * (c4 + (r1 * c5)))));
    const double pq  = z * q;
    const double rh  = (r1 + s35) - s35;
    const double rl  = r1 - rh;
    const double b   = ( (t2 * rh) + (thi * ((rl + _a.r2) + pq)) ) +
                       (tlo * (1.0 + r1));
    const struct dj_math_dd h = d_math_dd_fast_two_sum(thi, t1 * rh);
    const double lo  = h.low + b;
    struct dj_internal_math_d_exp_estimate result = { 0.0, 0.0, 0.0 };

    result.high = h.high;
    result.low  = lo;
    result.err  = (_a.k != 0.0)
        ? (h.high * (1.0 / 9223372036854775808.0))
        : ( (z * (1.0 / 70368744177664.0)) +
            ( (((lo < 0.0) ? -lo : lo) + ((b < 0.0) ? -b : b)) *
              (1.0 / 4503599627370496.0) ) );

    return result;
}

// d_internal_math_d_exp_phase1
//   exp(x) from _a in doubles: the estimate, rounded where its bound allows,
// in the normal range or the subnormals'.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_exp_phase1(
    struct dj_internal_math_d_exp_arg _a
)
{
    const struct dj_internal_math_d_exp_estimate one =
        d_internal_math_d_exp_estimate_of(_a);

    return d_internal_math_d_exp_decide(one.high, one.low, one.err, _a.e);
}

// d_internal_math_d_exp_phase2
//   exp(x) from _a in pairs of doubles: r = rH + rL exactly; e^rH - 1 by the
// polynomial rH + rH^2/2 + rH^3 P(rH), P of degree 6 (Sollya's), within
// 2^-110.3, its tail in doubles and its first coefficients as pairs; then
// rL (1 + E) and T (1 + E). Within 2^-101.2 of exp(x), absolutely, the
// result below 2.01 (proofs/exp_phase2.g): tested against 2^-100.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_exp_phase2(
    struct dj_internal_math_d_exp_arg _a
)
{
    const struct dj_math_dd a3 = {
        @A3H@,
        @A3L@
    };
    const struct dj_math_dd a4 = {
        @A4H@,
        @A4L@
    };
    const double a5 = @A5@;
    const double a6 = @A6@;
    const double a7 = @A7@;
    const double a8 = @A8@;
    const double a9 = @A9@;
    const struct dj_math_dd t  = { d_internal_math_d_exp_t(4 * _a.j),
                                   d_internal_math_d_exp_t((4 * _a.j) + 1) };
    const struct dj_math_dd r  = d_math_dd_two_sum(_a.r1, _a.r2);
    const double            rh = r.high;
    const double            tt =
        a5 + (rh * (a6 + (rh * (a7 + (rh * (a8 + (rh * a9)))))));
    // P = a3 + rH (a4 + rH tt), then Q = 1/2 + rH P, E = rH + rH^2 Q
    const struct dj_math_dd w  =
        d_math_dd_add(a4, d_math_dd_mul_d(d_math_dd_of(tt), rh));
    const struct dj_math_dd p  = d_math_dd_add(a3, d_math_dd_mul_d(w, rh));
    const struct dj_math_dd q  = d_math_dd_add_d(d_math_dd_mul_d(p, rh), 0.5);
    const struct dj_math_dd e  =
        d_math_dd_add_d(d_math_dd_mul(d_math_dd_two_product(rh, rh), q), rh);
    // + rL (1 + E), then T (1 + E)
    const struct dj_math_dd e2 = d_math_dd_add_d(e, r.low + (r.low * e.high));
    const struct dj_math_dd y  = d_math_dd_add(t, d_math_dd_mul(t, e2));

    return d_internal_math_d_exp_decide(y.high, y.low,
                                        1.0 / 1267650600228229401496703205376.0,
                                        _a.e);
}
"""

EXP_D_SECTION_3 = r"""
// The third phase computes in fixed point, exactly but for truncations it
// counts: a value is the integer of 16 limbs of 16 bits, limb i of weight
// 2^(16 i), times 2^-224 - limbs of 16 bits, so that two multiply into an
// unsigned long, which every language level has, and which holds a column's
// sum. ln2/128 is held to 240 bits, 2^(j/128) to 224, as digits (below);
// x - k ln2/128 is then within 3.1 units of 2^-224 of r, e^r's series is
// summed until its terms vanish, each truncation counted, and the table
// value multiplies it: within 51.4 units of 2^-224 of the exact value, on
// either side, which is less than 2^-218.3 of it (proofs/exp_phase3.py).
// That is far below the 2^-110.6 by which the hardest known argument's
// exponential misses a rounding boundary, relatively, so the computed value
// rounds as the exact one does.

// D_INTERNAL_MATH_D_EXP_DIGITS_A, D_INTERNAL_MATH_D_EXP_DIGITS_B
//   the third phase's digits, in hexadecimal: ln2/128 times 2^240, rounded
// down, in 60 digits, then for j from 0 to 127 2^(j/128) times 2^224, rounded
// down, in 57 digits each; the first 3,708 digits, to j = 63, in the first
// literal, the rest in the second, each within C99's 4,095 characters. String
// literals, as elementary.h's digits of 2/pi are, read only by the third
// phase.
#define D_INTERNAL_MATH_D_EXP_DIGITS_A                                         \
@DIGITS_A@

#define D_INTERNAL_MATH_D_EXP_DIGITS_B                                         \
@DIGITS_B@

// dj_internal_math_d_exp_fixed
//   struct: a nonnegative value, the sum of limb[i] 2^(16 i - 224), each
// limb below 2^16.
struct dj_internal_math_d_exp_fixed
{
    unsigned long limb[16];
};

// d_internal_math_d_exp_digit
//   the value of the hexadecimal digit at _at of the third phase's digits.
D_MATH_FN unsigned long
d_internal_math_d_exp_digit(
    int _at
)
{
    const char c = (_at < 3708) ? D_INTERNAL_MATH_D_EXP_DIGITS_A[_at]
                                : D_INTERNAL_MATH_D_EXP_DIGITS_B[_at - 3708];

    return (c <= '9') ? (unsigned long)(c - '0')
                      : (unsigned long)((c - 'a') + 10);
}

// d_internal_math_d_exp_fixed_read
//   the value of _count digits at _at, the last of weight 2^(16 _shift):
// limbs from the last digit up, each of 4 digits.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_fixed_read(
    int _at,
    int _count,
    int _shift
)
{
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    int i = 0;

    for (i = 0; i < _count; ++i)
    {
        const int place = (_count - 1) - i;
        const int limb  = _shift + (place / 4);

        result.limb[limb] |= d_internal_math_d_exp_digit(_at + i)
                             << (4 * (place % 4));
    }

    return result;
}

// d_internal_math_d_exp_fixed_carry
//   _limbs carried, from the first, so that each but the last is below 2^16.
D_MATH_FN void
d_internal_math_d_exp_fixed_carry(
    unsigned long* _limbs,
    int            _count
)
{
    unsigned long carry = 0;
    int           i     = 0;

    for (i = 0; i < _count; ++i)
    {
        const unsigned long v = _limbs[i] + carry;

        _limbs[i] = v & 0xFFFFul;
        carry     = v >> 16;
    }

    return;
}

// d_internal_math_d_exp_fixed_mul
//   _a _b, the product's lowest 224 bits dropped: truncated, below the exact
// product by less than 2^-224.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_fixed_mul(
    struct dj_internal_math_d_exp_fixed _a,
    struct dj_internal_math_d_exp_fixed _b
)
{
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    unsigned long column[33] = { 0 };
    int i = 0;
    int j = 0;

    for (i = 0; i < 16; ++i)
    {
        for (j = 0; j < 16; ++j)
        {
            const unsigned long p = _a.limb[i] * _b.limb[j];

            column[i + j]     += p & 0xFFFFul;
            column[i + j + 1] += p >> 16;
        }

        d_internal_math_d_exp_fixed_carry(column, 33);
    }

    for (i = 0; i < 16; ++i)
    {
        result.limb[i] = column[i + 14];
    }

    return result;
}

// d_internal_math_d_exp_fixed_div
//   _a / _k, truncated, _k small.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_fixed_div(
    struct dj_internal_math_d_exp_fixed _a,
    unsigned long                       _k
)
{
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    unsigned long rest = 0;
    int           i    = 0;

    for (i = 15; i >= 0; --i)
    {
        const unsigned long v = (rest << 16) | _a.limb[i];

        result.limb[i] = v / _k;
        rest           = v % _k;
    }

    return result;
}

// d_internal_math_d_exp_fixed_add
//   _a + _b, exactly.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_fixed_add(
    struct dj_internal_math_d_exp_fixed _a,
    struct dj_internal_math_d_exp_fixed _b
)
{
    int i = 0;

    for (i = 0; i < 16; ++i)
    {
        _a.limb[i] += _b.limb[i];
    }

    d_internal_math_d_exp_fixed_carry(_a.limb, 16);

    return _a;
}

// d_internal_math_d_exp_fixed_sub
//   _a - _b, exactly, _a >= _b.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_fixed_sub(
    struct dj_internal_math_d_exp_fixed _a,
    struct dj_internal_math_d_exp_fixed _b
)
{
    unsigned long borrow = 0;
    int           i      = 0;

    for (i = 0; i < 16; ++i)
    {
        const unsigned long need = _b.limb[i] + borrow;

        borrow     = (_a.limb[i] < need) ? 1ul : 0ul;
        _a.limb[i] = (_a.limb[i] + (borrow << 16)) - need;
    }

    return _a;
}

// d_internal_math_d_exp_fixed_less
//   whether _a < _b.
D_MATH_FN bool
d_internal_math_d_exp_fixed_less(
    struct dj_internal_math_d_exp_fixed _a,
    struct dj_internal_math_d_exp_fixed _b
)
{
    int i = 0;

    for (i = 15; i >= 0; --i)
    {
        if (_a.limb[i] != _b.limb[i])
        {
            return _a.limb[i] < _b.limb[i];
        }
    }

    return false;
}

// d_internal_math_d_exp_fixed_of
//   |_x| times 2^224, exactly, for 2^-105 <= |_x| < 2^16, whose bits all
// lie between 2^-224 and 2^16: peeled 16 at a time from the top, each
// division by a power of 2 exact.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_fixed_of(
    double _x
)
{
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    double rest = (_x < 0.0) ? -_x : _x;
    int    i    = 0;

    for (i = 15; i >= 0; --i)
    {
        // limb i's weight, 2^(16 i - 224), in two steps within the normal
        // range
        const double weight = d_internal_math_d_pow2((16 * i) - 112) *
                              d_internal_math_d_pow2(-112);
        const unsigned long digit = (unsigned long)(rest / weight);

        result.limb[i] = digit;
        rest          -= (double)digit * weight;
    }

    return result;
}

// d_internal_math_d_exp_fixed_bit
//   bit _k of _a, 0 below bit 0.
D_MATH_FN unsigned long
d_internal_math_d_exp_fixed_bit(
    struct dj_internal_math_d_exp_fixed _a,
    int                                 _k
)
{
    return (_k < 0) ? 0ul : ((_a.limb[_k / 16] >> (_k % 16)) & 1ul);
}

// d_internal_math_d_exp_fixed_bits
//   the number of bits of _a.
D_MATH_FN int
d_internal_math_d_exp_fixed_bits(
    struct dj_internal_math_d_exp_fixed _a
)
{
    int i = 0;
    int b = 0;

    for (i = 15; i >= 0; --i)
    {
        if (_a.limb[i] != 0)
        {
            while ((_a.limb[i] >> b) != 0)
            {
                ++b;
            }

            return (16 * i) + b;
        }
    }

    return 0;
}

// d_internal_math_d_exp_fixed_round
//   _a rounded to the nearest multiple of 2^_s, ties to even, as that
// multiple's integer, below 2^54: a double, exactly.
D_MATH_FN double
d_internal_math_d_exp_fixed_round(
    struct dj_internal_math_d_exp_fixed _a,
    int                                 _s
)
{
    double m      = 0.0;
    bool   sticky = false;
    int    k      = 0;

    for (k = d_internal_math_d_exp_fixed_bits(_a) - 1; k >= _s; --k)
    {
        m = (2.0 * m) + (double)d_internal_math_d_exp_fixed_bit(_a, k);
    }

    for (k = 0; k < (_s - 1); ++k)
    {
        sticky = sticky || (d_internal_math_d_exp_fixed_bit(_a, k) != 0);
    }

    if ( (_s > 0) &&
         (d_internal_math_d_exp_fixed_bit(_a, _s - 1) != 0) &&
         ( sticky || (d_internal_math_d_exp_fixed_bit(_a, _s) != 0) ) )
    {
        m += 1.0;
    }

    return m;
}

// d_internal_math_d_exp_series
//   e^r times 2^224, r = _r 2^-224 of sign _negative, |r| < 2^-8: the series
// summed until a term is 0, its positive and negative terms apart, each
// term truncated, below its exact value by at most 1.5 units, and the terms
// dropped below 1.007 units in all (proofs/exp_phase3.py).
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_series(
    struct dj_internal_math_d_exp_fixed _r,
    bool                                _negative
)
{
    struct dj_internal_math_d_exp_fixed term =
        d_internal_math_d_exp_fixed_of(1.0);
    struct dj_internal_math_d_exp_fixed up   = term;
    struct dj_internal_math_d_exp_fixed down = { { 0 } };
    unsigned long k = 1;

    for (k = 1; k < 64; ++k)
    {
        term = d_internal_math_d_exp_fixed_div(
            d_internal_math_d_exp_fixed_mul(term, _r), k);

        if (d_internal_math_d_exp_fixed_bits(term) == 0)
        {
            break;
        }

        if (_negative && ((k % 2) == 1))
        {
            down = d_internal_math_d_exp_fixed_add(down, term);
        }
        else
        {
            up = d_internal_math_d_exp_fixed_add(up, term);
        }
    }

    return d_internal_math_d_exp_fixed_sub(up, down);
}

// d_internal_math_d_exp_ln2_times
//   |_k| ln2/128 times 2^224, |_k| < 2^18: ln2/128's 240-bit digits times
// |_k|, in two 16-bit parts, then 16 bits dropped; below the exact value by
// less than 2^18 2^-240 + 2^-224.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_exp_ln2_times(
    double _k
)
{
    const struct dj_internal_math_d_exp_fixed l =
        d_internal_math_d_exp_fixed_read(0, 60, 0);
    const unsigned long n  = (unsigned long)((_k < 0.0) ? -_k : _k);
    const unsigned long nl = n & 0xFFFFul;
    const unsigned long nh = n >> 16;
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    unsigned long column[18] = { 0 };
    int i = 0;

    for (i = 0; i < 16; ++i)
    {
        const unsigned long p = l.limb[i] * nl;
        const unsigned long q = l.limb[i] * nh;

        column[i]     += p & 0xFFFFul;
        column[i + 1] += (p >> 16) + (q & 0xFFFFul);
        column[i + 2] += q >> 16;
        d_internal_math_d_exp_fixed_carry(column, 18);
    }

    for (i = 0; i < 16; ++i)
    {
        result.limb[i] = column[i + 1];
    }

    return result;
}

// d_internal_math_d_exp_phase3
//   exp(x) from _x and _a in fixed point, correctly rounded: r = x - k ln2/128
// from x exactly and k ln2/128 truncated, both of x's sign; e^r by its
// series; times the table value, within 51.4 units of 2^-224 of the exact
// value; then rounded at 53 bits, or at the least subnormal, as the exact
// value is: no rounding boundary is known to lie so close (section 3.2's
// note).
D_MATH_FN double
d_internal_math_d_exp_phase3(
    double                            _x,
    struct dj_internal_math_d_exp_arg _a
)
{
    const struct dj_internal_math_d_exp_fixed x =
        d_internal_math_d_exp_fixed_of(_x);
    const struct dj_internal_math_d_exp_fixed c =
        d_internal_math_d_exp_ln2_times(_a.k);
    const bool below = d_internal_math_d_exp_fixed_less(x, c);
    const struct dj_internal_math_d_exp_fixed r =
        below ? d_internal_math_d_exp_fixed_sub(c, x)
              : d_internal_math_d_exp_fixed_sub(x, c);
    const struct dj_internal_math_d_exp_fixed s =
        d_internal_math_d_exp_series(r, (_x < 0.0) != below);
    const struct dj_internal_math_d_exp_fixed y =
        d_internal_math_d_exp_fixed_mul(
            d_internal_math_d_exp_fixed_read(60 + (57 * _a.j), 57, 0), s);
    int place = d_internal_math_d_exp_fixed_bits(y) - 53;

    // the least subnormal's place, where the result is below 2^-1022
    if ( (_a.e + ((d_internal_math_d_exp_fixed_bits(y) - 1) - 224)) < -1022 )
    {
        place = 224 - 1074 - _a.e;
    }

    {
        const double m = d_internal_math_d_exp_fixed_round(y, place);
        // m 2^(place - 224 + e), in two steps within the normal range
        const int e = (place - 224) + _a.e;

        return (e < -1022)
            ? ((m * d_internal_math_d_exp_pow2(e + 1022)) *
               d_internal_math_d_exp_pow2(-1022))
            : d_internal_math_d_exp_times_pow2(m, e);
    }
}

// d_internal_math_d_exp_slow
//   exp(_x) by the second phase and, where it does not decide, the third:
// the some 0.2% of arguments the first leaves. Out of line, so that exp_c's
// common path, inlined into its callers, keeps the small frame the first
// phase needs.
D_MATH_FN_COLD double
d_internal_math_d_exp_slow(
    double _x
)
{
    const struct dj_internal_math_d_exp_arg a =
        d_internal_math_d_exp_arg_of(_x);
    const struct dj_internal_math_d_exp_result two =
        d_internal_math_d_exp_phase2(a);

    return two.decided ? two.value : d_internal_math_d_exp_phase3(_x, a);
}

// d_math_d_exp_c
//   e^_x, correctly rounded; see above. Where 2^-53 < |x| <= 708, |k| is at
// most 130743, so e runs from -1022 to 1021, and e^x is at least 2^-1021.4:
// the first phase's value low 2^e, low in [0.99, 2.01], is normal and
// finite, the product exact.
D_MATH_FN double
d_math_d_exp_c(
    double _x
)
{
    // the last argument whose exponential is finite, and the first whose
    // exponential rounds to more than 0
    const double last  = (6243314768165359.0 / 8796093022208.0);
    const double first = -(6554261109157969.0 / 8796093022208.0);
    // 2^-53: below it, and down to -2^-53, 1 + x is the correct rounding
    const double tiny  = 1.0 / 9007199254740992.0;
    // x^2, rounded: above 2^-106 and at most 708^2 just where 2^-53 < |x| <=
    // 708, as both are squares of doubles and rounding keeps the order; no
    // test of x's sign, which a caller's arguments may not let a processor
    // predict
    const double xx    = _x * _x;

    D_MATH_DBL_REFUSE(_x)

    // the common path: no NaN, and the result normal
    if ( (xx > (tiny * tiny)) && (xx <= 501264.0) )
    {
        const struct dj_internal_math_d_exp_arg a =
            d_internal_math_d_exp_arg_of(_x);
        const struct dj_internal_math_d_exp_estimate one =
            d_internal_math_d_exp_estimate_of(a);
        const double low  = one.high + (one.low - one.err);
        const double high = one.high + (one.low + one.err);

        return (low == high) ? (low * d_internal_math_d_exp_pow2(a.e))
                             : d_internal_math_d_exp_slow(_x);
    }

    if (_x != _x)
    {
        return _x;
    }

    if (_x > last)
    {
        return d_internal_math_d_infinity();
    }

    if (_x < first)
    {
        return 0.0;
    }

    if ( (_x < tiny) && (_x >= -tiny) )
    {
        return 1.0 + _x;
    }

    // near the ends of the range, where the result may be subnormal, and at
    // 2^-53 itself
    {
        const struct dj_internal_math_d_exp_arg a =
            d_internal_math_d_exp_arg_of(_x);
        const struct dj_internal_math_d_exp_result one =
            d_internal_math_d_exp_phase1(a);

        return one.decided ? one.value : d_internal_math_d_exp_slow(_x);
    }
}

// d_math_d_exp
//   e^_x, correctly rounded: exp_c; the C library's where double is
// evaluated wider than double; see above.
D_MATH_FN_RT double
d_math_d_exp(
    double _x
)
{
#if !D_MATH_DBL_EVAL_WIDE
    return d_math_d_exp_c(_x);
#elif defined(__cplusplus)
    return std::exp(_x);
#else
    return exp(_x);
#endif  // !D_MATH_DBL_EVAL_WIDE
}

// d_math_d_exp_fast
//   e^_x within 0.511 of an ulp, at the C library's time; see above. r is
// x - k L1 - k L2F rounded, within 2^-62 of x - k ln2/128, |r| < 0.00271;
// T (1 + p) is thi + (tlo + thi p), within 2^-58.5 of it before its last
// rounding where it is at least 1, and 2^-59.7 where it is below; the
// product by 2^e is exact, the result normal and finite, as exp_c's is.
D_MATH_FN_RT double
d_math_d_exp_fast(
    double _x
)
{
#if !D_MATH_DBL_EVAL_WIDE
    // 128/ln2; ln2/128 = L1 + L2F but for 2^-98.4, L1 of 35 bits
    const double invl  = @INVL@;
    const double l1    = @L1@;
    const double l2    = @L2F@;
    const double c2    = @C2@;
    const double c3    = @C3@;
    const double c4    = @C4@;
    const double c5    = @C5@;
    const double shift = 6755399441055744.0;
    // 2^-53, and x^2: 2^-53 < |x| <= 708 just where 2^-106 < x^2 <= 708^2,
    // as exp_c tests it
    const double tiny  = 1.0 / 9007199254740992.0;
    const double xx    = _x * _x;

    if ( (xx > (tiny * tiny)) && (xx <= 501264.0) )
    {
        const double t   = _x * invl;
        const double k   = (t + shift) - shift;
        const int    kb  = (int)k + 262144;
        const int    j   = kb & 127;
        const int    e   = (kb >> 7) - 2048;
        const double r   = (_x - (k * l1)) - (k * l2);
        const double thi = d_internal_math_d_exp_t(4 * j);
        const double tlo = d_internal_math_d_exp_t((4 * j) + 1);
        const double z   = r * r;
        const double p   = r + ( (z * (c2 + (r * c3))) +
                                 ((z * z) * (c4 + (r * c5))) );

        return (thi + (tlo + (thi * p))) * d_internal_math_d_exp_pow2(e);
    }

    return d_math_d_exp_c(_x);
#elif defined(__cplusplus)
    return std::exp(_x);
#else
    return exp(_x);
#endif  // !D_MATH_DBL_EVAL_WIDE
}
"""
