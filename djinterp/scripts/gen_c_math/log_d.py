"""gen_c_math.log_d - the text of log_d.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .power import (power)
from .exp_d import (exp_d_digits_literal, exp_d_hex_literal, exp_d_literal,
                    exp_d_round)


# -----------------------------------------------------------------------------
# exponential.h: the double family's natural logarithm, correctly rounded on
# doubles and pairs of doubles (chunk 52, S424)
# -----------------------------------------------------------------------------
# The constants are computed here with integers alone, each rounding checked
# to be the same at both ends of an interval that holds the exact value; the
# polynomials' coefficients are Sollya's (chunk 52's record,
# proofs/log_poly.sollya), and their error bounds and the three phases' are
# proven there (proofs/).

LOG_D_BITS = 520


def log_d_ln_fixed(num, den):
    """ln(num/den) times 2^LOG_D_BITS, num and den positive and num/den in
    [1/2, 2], as (value, error): 2 atanh(s), s = (num - den)/(num + den), its
    series in integers, each term truncated; the value within error units of
    the exact one."""
    a, d = abs(num - den), num + den
    one = 1 << LOG_D_BITS
    s = (a << LOG_D_BITS) // d          # below s 2^B by less than 1
    s2 = (a * a << LOG_D_BITS) // (d * d)
    total, power, k, cuts = 0, s, 1, 1
    while power:
        total += power // k
        power = (power * s2) >> LOG_D_BITS
        k += 2
        cuts += 2
    value = 2 * total
    # each power below its exact value by at most the number of steps so far,
    # each quotient by 1 more, the dropped tail below 1, all doubled
    error = 2 * (cuts * cuts + 2)
    return (value if num >= den else -value), error


def log_d_round_interval(lo, hi, scale, bits=53):
    """the value in [lo, hi] 2^-scale rounded to bits bits, as (m, e), checked
    to be the same at both ends."""
    a = exp_d_round(lo, 1 << scale, bits)
    b = exp_d_round(hi, 1 << scale, bits)
    assert a == b, "a rounding the interval leaves undecided"
    return a


# the inverses' significant bits: at most 26, so that m inv is exact by
# Dekker's product with m alone split (S432)
LOG_D_INV_BITS = 26


def log_d_constants():
    """ln2 in parts, and the table: for i from 0 to 128 the reciprocal inv_i
    of c_i = 1 + i/128 (of c_i/2 from i = 53) rounded to LOG_D_INV_BITS
    bits, 1 at 0 and 128, stored halved from 53 (so that m need not be),
    and -ln(inv_i) as a pair; also |-ln(inv_i)| 2^224 rounded down, as
    hexadecimal digits."""
    from fractions import Fraction
    B = LOG_D_BITS
    ln2, e2 = log_d_ln_fixed(2, 1)
    consts = {}
    parts = []
    rest_lo, rest_hi = ln2 - e2, ln2 + e2
    for name, bits in (("LN2H", 42), ("LN2M", 42), ("LN2L", 53)):
        m, e = log_d_round_interval(rest_lo, rest_hi, B, bits)
        v = Fraction(m) * Fraction(2) ** e
        consts[name] = exp_d_literal(m, e)
        parts.append(v)
        rest_lo -= int(v * (1 << B))
        rest_hi -= int(v * (1 << B))
    # phase 1's: ln2 - LN2H rounded to 53 bits
    vh = int(parts[0] * (1 << B))
    consts["LN2L1"] = exp_d_literal(*log_d_round_interval(ln2 - e2 - vh,
                                                          ln2 + e2 - vh, B))
    rows, digits = [], []
    for i in range(129):
        if i in (0, 128):
            inv = Fraction(1)
        else:
            num = 128 if i < 53 else 256
            m, e = exp_d_round(num, 128 + i, LOG_D_INV_BITS)
            inv = Fraction(m) * Fraction(2) ** e
        stored = inv / 2 if i >= 53 else inv
        if inv == 1:
            rows.append(("1.0" if i == 0 else exp_d_literal(1, -1), "0.0",
                         "0.0"))
            digits.append("0" * 56)
            continue
        # -ln(inv) = ln(1/inv)
        L, err = log_d_ln_fixed(inv.denominator, inv.numerator)
        mh, eh = log_d_round_interval(L - err, L + err, B)
        vh = int(Fraction(mh) * Fraction(2) ** eh * (1 << B))
        ml, el = log_d_round_interval(L - err - vh, L + err - vh, B)
        rows.append((exp_d_literal(*exp_d_round(stored.numerator,
                                                stored.denominator)),
                     exp_d_literal(mh, eh), exp_d_literal(ml, el)))
        lo_d = (abs(L) - err) >> (B - 224)
        hi_d = (abs(L) + err) >> (B - 224)
        assert lo_d == hi_d, "a digit the interval leaves undecided"
        digits.append("%056x" % lo_d)
    return consts, rows, "".join(digits)


# Sollya's coefficients (proofs/log_poly.out): phase 1's q, degree 6, with
# log1p(r) = r + r^2 q(r) (S432); phase 2's P, degree 10, its first five
# coefficients as pairs
LOG_D_Q = ["-0x1p-1", "0x1.5555555555556p-2", "-0x1.0000000000002p-2",
           "0x1.99999998dddd4p-3", "-0x1.5555555420254p-3",
           "0x1.2493cf3e41b7fp-3", "-0x1.0001c201fc422p-3"]
LOG_D_P = [("0x1.5555555555555p-2", "0x1.5555555555555p-56"),
           ("-0x1p-2", "-0x1.92438p-91"),
           ("0x1.999999999999ap-3", "-0x1.9999999760c7p-57"),
           ("-0x1.5555555555555p-3", "-0x1.554d79f1fd72cp-57"),
           ("0x1.2492492492492p-3", "0x1.247c7c5ea07ap-57"),
           ("-0x1.000000000000bp-3", None), ("0x1.c71c71c71c745p-4", None),
           ("-0x1.999999967504fp-4", None), ("0x1.745d174176c9ap-4", None),
           ("-0x1.555879eab4efap-4", None), ("0x1.3b17310bb3497p-4", None)]

# the bounds proven in chunks 52's and 54's records (proofs/)
LOG_D_BOUNDS = {"p1a": "2^-66.4", "p1": "2^-59.9", "qerr": "2^-65.05",
                "ka": 90, "kappa": (11, 1 << 55),
                "kappa_text": "11 2^-55",
                "p2": "2^-101.4", "p3": "2^-165.8", "p3u": "36.1"}


def log_d_table_macro(rows):
    """the table as a macro's initializer, one value a line."""
    out = ["#define D_INTERNAL_MATH_D_LN_TABLE" + " " * 45 + "\\",
           "    {" + " " * 74 + "\\"]
    values = [v for row in rows for v in row]
    for k, v in enumerate(values):
        text = "        " + v + ("" if k + 1 == len(values) else ",")
        out.append(text + " " * (79 - len(text)) + "\\")
    out.append("    }")
    return "\n".join(out)

LOG_D_SECTION = r"""// 3.3    Natural logarithm
//------------------------------------------------------------------------------
// log_c is the natural logarithm correctly rounded, in double arithmetic
// alone - no long double and no C library - and a constant expression from
// C++14 (S424). x = m 2^e, m in [1, 2), a subnormal first lifted by 2^54; i,
// from 0 to 128, picks the c = 1 + i/128 nearest m, and from c > sqrt(2), i
// from 53, c is halved and e raised by 1; then log(x) = e ln2 + L +
// log1p(r), with inv a number of 26 bits within 2^-26 of 1/c, relatively,
// L = -log(inv) from a table of 129 triples, the table holding inv halved
// from 53 so that m is never halved, and r = m inv - 1 exactly as a pair,
// m inv taken by Dekker's product with m alone split, |r| < 2^-8 (S432).
// Three phases follow, each with a bound proven in chunks 52's and 54's
// records (proofs/), each giving its result where the interval its bound
// allows rounds one way, as Ziv's method has it: phase 1, in doubles, within
// 2^-@KA@ of the result plus @KAPPA_TEXT@ rh^2, @B1@ of the logarithm at
// worst, answers all but some 0.4% of arguments within 2^-7 of 1 and nearly
// every other; phase 2, in pairs of doubles (dd.h), within @B2@; phase 3, in
// fixed point of 256 bits, within @B3@, far closer than a double's logarithm
// is known to come to a rounding boundary - the hardest case known, Lefevre
// and Muller's x = 0x1.62a88613629b6p+678, lies 2^-118.0 of its logarithm
// from one. The bounds hold whether or not the compiler fuses a multiply
// and an add (math_common.h, section 5). NaN and +inf are their own
// logarithms, log(+-0) is -inf, a negative x gives NaN, and log(1) is +0.
// Where double is evaluated wider than double, a 32-bit x87, log_c refuses
// to compile: build with -msse2 -mfpmath=sse (math_common.h,
// D_MATH_DBL_REFUSE). The plain log is log_c, as ruling 12 has a plain
// double function once its correctly rounded form is within about twice the
// C library's time (S496), but where double is evaluated wider than double,
// where it is the C library's (S497).

// D_INTERNAL_MATH_D_LN_TABLE
//   for i from 0 to 128, at 3 i: inv, 1 at 0 and 128, otherwise 1/c (below
// 53) or 2/c (from 53) rounded to 26 significant bits, halved from 53 (S432);
// at 3 i + 1 and 3 i + 2 -log(inv), inv unhalved, as a pair, within 2^-106 of
// it, relatively; each exact as written (chunk 54's record,
// proofs/log_consts.py).
@TABLE@

#ifdef __cplusplus
// dj_internal_math_d_ln_table
//   struct: the table as a template's static member, as exp_c's is (S420).
template <int Unused>
struct dj_internal_math_d_ln_table
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr double value[387] = D_INTERNAL_MATH_D_LN_TABLE;
#else
    static const double value[387];
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
};

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
template <int Unused>
const double dj_internal_math_d_ln_table<Unused>::value[387] =
    D_INTERNAL_MATH_D_LN_TABLE;
#elif !D_ENV_LANG_IS_CPP17_OR_HIGHER
template <int Unused>
constexpr double dj_internal_math_d_ln_table<Unused>::value[387];
#endif
#endif  // __cplusplus

// d_internal_math_d_ln_t
//   the table's _index-th value. In C a function's own table, so that a
// translation unit that never reaches it holds none.
D_MATH_FN double
d_internal_math_d_ln_t(
    int _index
)
{
#ifdef __cplusplus
    return dj_internal_math_d_ln_table<0>::value[_index];
#else
    static const double table[387] = D_INTERNAL_MATH_D_LN_TABLE;

    return table[_index];
#endif  // __cplusplus
}

// dj_internal_math_d_ln_split
//   struct: a positive normal double as m 2^e, m in [1, 2); i, the integer
// nearest 128 (m - 1); and mh, m's high part, by which m inv is taken
// exactly (d_internal_math_d_ln_product).
struct dj_internal_math_d_ln_split
{
    double m;
    double mh;
    int    e;
    int    i;
};

#if ( ( !defined(__cplusplus) && defined(UINT64_MAX) ) ||                     \
      ( defined(__cplusplus) && defined(UINT64_MAX) &&                         \
        defined(D_INTERNAL_MATH_HAS_BIT_CAST) ) )
    // D_INTERNAL_MATH_D_LN_BITS
    //   1 where log_c reads a double's bits: in C with a 64-bit integer, by a
    // union, and in C++ by __builtin_bit_cast, a constant expression. A test
    // may define it 0 first, to take the other path where it is 1.
    #ifndef D_INTERNAL_MATH_D_LN_BITS
        #define D_INTERNAL_MATH_D_LN_BITS 1
    #endif
#else
    #undef D_INTERNAL_MATH_D_LN_BITS
    #define D_INTERNAL_MATH_D_LN_BITS 0
#endif

#if D_INTERNAL_MATH_D_LN_BITS
// d_internal_math_d_ln_bits
//   _v's bits.
D_MATH_FN uint64_t
d_internal_math_d_ln_bits(
    double _v
)
{
#ifdef __cplusplus
    return __builtin_bit_cast(uint64_t, _v);
#else
    union
    {
        uint64_t bits;
        double   value;
    } v = { 0 };

    v.value = _v;

    return v.bits;
#endif  // __cplusplus
}

// d_internal_math_d_ln_double
//   the double whose bits are _bits.
D_MATH_FN double
d_internal_math_d_ln_double(
    uint64_t _bits
)
{
#ifdef __cplusplus
    return __builtin_bit_cast(double, _bits);
#else
    union
    {
        uint64_t bits;
        double   value;
    } v = { 0 };

    v.bits = _bits;

    return v.value;
#endif  // __cplusplus
}
#endif  // D_INTERNAL_MATH_D_LN_BITS

// d_internal_math_d_ln_split
//   a positive normal _v as m 2^e, m in [1, 2), exactly, with i and mh.
// Where the language reads a double's bits, from them alone: m's 52 bits
// below its leading one are f, so that 128 (m - 1) + 1/2 is (f + 2^44)
// 2^-45 and i that shifted by 45, and mh is m with f's last 26 bits cleared,
// of 27 significant bits, m - mh of at most 26. Elsewhere by float_order.h's
// binade search, i from 128 (m - 1) + 1/2, a multiple of 2^-45 below 128.5,
// and mh by Veltkamp's split, of 26 bits (S495).
D_MATH_FN struct dj_internal_math_d_ln_split
d_internal_math_d_ln_split(
    double _v
)
{
    struct dj_internal_math_d_ln_split result = { 1.0, 1.0, 0, 0 };
#if D_INTERNAL_MATH_D_LN_BITS
    const uint64_t bits = d_internal_math_d_ln_bits(_v);
    const uint64_t f    = bits & ((((uint64_t)1) << 52) - 1);
    const uint64_t one  = ((uint64_t)1023) << 52;

    result.e  = (int)((bits >> 52) & 0x7FFu) - 1023;
    result.i  = (int)((f + (((uint64_t)1) << 44)) >> 45);
    result.m  = d_internal_math_d_ln_double(f | one);
    result.mh = d_internal_math_d_ln_double(
        (f & ~((((uint64_t)1) << 26) - 1)) | one);
#else
    const struct dj_internal_math_d_binade b =
        d_internal_math_d_binade_of(_v);
    const double m = _v / b.power;
    const double t = d_internal_math_d_veltkamp() * m;

    result.e  = b.exponent;
    result.i  = (int)(((m - 1.0) * 128.0) + 0.5);
    result.m  = m;
    result.mh = t - (t - m);
#endif  // D_INTERNAL_MATH_D_LN_BITS

    return result;
}

// d_internal_math_d_ln_normal
//   whether _v is a positive normal finite double, every double but the
// rest: from its bits by one unsigned comparison where the language reads
// them, by two comparisons elsewhere (S495).
D_MATH_FN bool
d_internal_math_d_ln_normal(
    double _v
)
{
#if D_INTERNAL_MATH_D_LN_BITS
    return ( (d_internal_math_d_ln_bits(_v) - (((uint64_t)1) << 52)) <
             (((uint64_t)0x7FE) << 52) );
#else
    return ( (_v >= DBL_MIN) &&
             (_v <= DBL_MAX) );
#endif  // D_INTERNAL_MATH_D_LN_BITS
}

// dj_internal_math_d_ln_arg
//   struct: x reduced: log(x) = e ln2 + L_i + log1p(rh + rl), exactly, L_i =
// -log(inv_i); |rh + rl| < 2^-8, rl within half of rh's last place.
struct dj_internal_math_d_ln_arg
{
    int    e;
    int    i;   // 0 to 128
    double rh;
    double rl;
};

// d_internal_math_d_ln_product
//   _m _inv exactly, as high + low, high its rounding, _inv of at most 26
// significant bits: the low part by a fused multiply-add where the target
// has one, and otherwise by Dekker's product with _m alone split, at _mh,
// so that both give the same bits as d_math_dd_two_product (S432). _mh of
// at most 27 significant bits and _m - _mh of at most 26, each product by
// _inv is exact; mh inv less high is a multiple of 2^-53 below 2^-25, and
// exact; that plus (m - mh) inv is m inv - high, a multiple of 2^-79 at most
// 2^-53, exact too (S495; chunk 66's record, proofs/).
D_MATH_FN struct dj_math_dd
d_internal_math_d_ln_product(
    double _m,
    double _mh,
    double _inv
)
{
    struct dj_math_dd result = { 0.0, 0.0 };

#if D_MATH_DBL_FMA
    if (D_MATH_RUN_TIME())
    {
        result.high = _m * _inv;
        result.low  = D_MATH_DBL_FMA_OF(_m, _inv, -result.high);

        return result;
    }
#endif  // D_MATH_DBL_FMA

    {
        const double high = _m * _inv;

        result.high = high;
        result.low  = ((_mh * _inv) - high) + ((_m - _mh) * _inv);

        return result;
    }
}

// d_internal_math_d_ln_arg_of
//   _v 2^_lift, _v positive, normal and finite, reduced. i is the integer
// nearest (m - 1) 128, so that |m - c| <= 1/256. From i = 53 the table holds
// inv halved, so that m times it is m/2 times inv, exactly, and e is raised
// by 1 (S432). m inv is exact as a pair; its high part lies within 2^-7 of
// 1, so that it minus 1 is exact (Sterbenz's lemma), and at least the low
// part's place, or 0, so that Fast2Sum is exact: r = m inv - 1, a multiple
// of 2^-106, of which rh and rl are each a multiple too.
D_MATH_FN struct dj_internal_math_d_ln_arg
d_internal_math_d_ln_arg_of(
    double _v,
    int    _lift
)
{
    const struct dj_internal_math_d_ln_split s =
        d_internal_math_d_ln_split(_v);
    const int    i = s.i;
    const struct dj_math_dd p = d_internal_math_d_ln_product(
        s.m, s.mh, d_internal_math_d_ln_t(3 * i));
    const struct dj_math_dd r = d_math_dd_fast_two_sum(p.high - 1.0, p.low);
    struct dj_internal_math_d_ln_arg result = { 0, 0, 0.0, 0.0 };

    result.e  = (s.e + _lift) + ((i >= 53) ? 1 : 0);
    result.i  = i;
    result.rh = r.high;
    result.rl = r.low;

    return result;
}

// d_internal_math_d_ln_decide
//   _yh + _yl rounded once, where the exact value is within _err of it less
// the rounding of _yl -+ _err, which the bounds leave room for: decided
// where both ends of the interval round alike. The logarithm of a double is
// neither subnormal nor past the largest double.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_ln_decide(
    double _yh,
    double _yl,
    double _err
)
{
    struct dj_internal_math_d_exp_result result = { false, 0.0 };
    const double low  = _yh + (_yl - _err);
    const double high = _yh + (_yl + _err);

    if (low == high)
    {
        result.decided = true;
        result.value   = low;
    }

    return result;
}

// d_internal_math_d_ln_phase1
//   log(x) from _a in doubles: E = e LN2H, exact (LN2H of 42 bits, |e| <
// 1100), and L_i's high part, then rh, taken exactly by Fast2Sum, |E| >=
// |L_i| where e is not 0 and |L_i| >= 2^-8 > |rh| where i is neither 0 nor
// 128, either 0 otherwise; log1p(rh) = rh + rh^2 q(rh), q of degree 6
// (Sollya's), within @QERR@ of (log1p(rh) - rh)/rh^2; rl adds rl (1 - rh);
// the low parts summed (S432). The computed pair lies within 2^-@KA@ of the
// result plus @KAPPA_TEXT@ rh^2 of log(x), less the rounding of lo -+ err,
// in each of three cases - e not 0; e 0 and i neither 0 nor 128; e 0 and
// i 0 or 128, where r = x - 1 - (proofs/log_phase1_*.g): within @B1A@ of
// it, relatively, where e is not 0, and @B1@ in the worst case.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_ln_phase1(
    struct dj_internal_math_d_ln_arg _a
)
{
    // ln2's first 42 bits, and the rest rounded
    const double ln2h  = @LN2H@;
    const double ln2l1 = @LN2L1@;
    const double q0    = @Q0@;
    const double q1    = @Q1@;
    const double q2    = @Q2@;
    const double q3    = @Q3@;
    const double q4    = @Q4@;
    const double q5    = @Q5@;
    const double q6    = @Q6@;
    const double e     = (double)_a.e;
    const double rh    = _a.rh;
    const double rl    = _a.rl;
    const struct dj_math_dd a = d_math_dd_fast_two_sum(
        e * ln2h, d_internal_math_d_ln_t((3 * _a.i) + 1));
    const struct dj_math_dd b = d_math_dd_fast_two_sum(a.high, rh);
    const double r2    = rh * rh;
    const double q     =
        q0 + (rh * (q1 + (rh * (q2 + (rh * (q3 + (rh * (q4 + (rh * (q5 +
        (rh * q6)))))))))));
    const double lo    =
        ( ( (a.low + b.low) + (d_internal_math_d_ln_t((3 * _a.i) + 2) +
                               (e * ln2l1)) ) +
          (rl * (1.0 - rh)) ) +
        (r2 * q);
    // the test's room: 2^-@KA@ of the result, and @KAPPA_TEXT@ of rh^2, the
    // part of the error that grows with it (S432)
    const double room1 = @EA@;
    const double room2 = @KAPPA@;
    // |b.high| as the greater of it and its negation, a negation and a
    // maximum where a comparison took five instructions (S495)
    const double err   = (((b.high > -b.high) ? b.high : -b.high) * room1) +
                         (r2 * room2);

    return d_internal_math_d_ln_decide(b.high, lo, err);
}

// d_internal_math_d_ln_phase2
//   log(x) from _a in pairs of doubles: log1p(rh) = rh - rh^2/2 + rh^3
// P(rh), P of degree 10 (Sollya's), within 2^-101.8, its tail in doubles and
// its first coefficients as pairs; rl adds rl/(1 + rh); e ln2 from ln2 in
// three parts, the first two of 42 bits, so that their products by e are
// exact; then e ln2 + L_i, and the logarithm of 1 + r added. Within
// @B2@ of log(x), relatively, in each of phase 1's three cases (proofs/
// log_phase2_*.g): tested against 2^-100 times the result's high part.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_ln_phase2(
    struct dj_internal_math_d_ln_arg _a
)
{
    const double ln2h = @LN2H@;
    const double ln2m = @LN2M@;
    const double ln2l =
        @LN2L@;
    const struct dj_math_dd p0 = {
        @P0H@,
        @P0L@
    };
    const struct dj_math_dd p1 = {
        @P1H@,
        @P1L@
    };
    const struct dj_math_dd p2 = {
        @P2H@,
        @P2L@
    };
    const struct dj_math_dd p3 = {
        @P3H@,
        @P3L@
    };
    const struct dj_math_dd p4 = {
        @P4H@,
        @P4L@
    };
    const double p5 = @P5@;
    const double p6 = @P6@;
    const double p7 = @P7@;
    const double p8 = @P8@;
    const double p9 = @P9@;
    const double p10 = @P10@;
    const double e  = (double)_a.e;
    const double rh = _a.rh;
    const double t  =
        p5 + (rh * (p6 + (rh * (p7 + (rh * (p8 + (rh * (p9 + (rh * p10)))))))));
    // P = p0 + rh (p1 + rh (p2 + rh (p3 + rh (p4 + rh t))))
    const struct dj_math_dd w4 =
        d_math_dd_add(p4, d_math_dd_mul_d(d_math_dd_of(t), rh));
    const struct dj_math_dd w3 = d_math_dd_add(p3, d_math_dd_mul_d(w4, rh));
    const struct dj_math_dd w2 = d_math_dd_add(p2, d_math_dd_mul_d(w3, rh));
    const struct dj_math_dd w1 = d_math_dd_add(p1, d_math_dd_mul_d(w2, rh));
    const struct dj_math_dd w0 = d_math_dd_add(p0, d_math_dd_mul_d(w1, rh));
    const struct dj_math_dd r2 = d_math_dd_two_product(rh, rh);
    const struct dj_math_dd r3 = d_math_dd_mul_d(r2, rh);
    // -rh^2/2, exactly
    const struct dj_math_dd h  = { r2.high * -0.5, r2.low * -0.5 };
    // rh - rh^2/2 + rh^3 P, then + rl/(1 + rh)
    const struct dj_math_dd l1 = d_math_dd_add_d(
        d_math_dd_add(d_math_dd_add_d(h, rh), d_math_dd_mul(r3, w0)),
        _a.rl / (1.0 + rh));
    // e ln2, then + L_i
    const struct dj_math_dd el = d_math_dd_add_d(
        d_math_dd_two_sum(e * ln2h, e * ln2m), e * ln2l);
    const struct dj_math_dd tl = { d_internal_math_d_ln_t((3 * _a.i) + 1),
                                   d_internal_math_d_ln_t((3 * _a.i) + 2) };
    const struct dj_math_dd y  = d_math_dd_add(d_math_dd_add(el, tl), l1);

    return d_internal_math_d_ln_decide(
        y.high, y.low,
        ((y.high < 0.0) ? -y.high : y.high) *
            (1.0 / 1267650600228229401496703205376.0));
}
"""

LOG_D_SECTION_3 = r"""
// The third phase computes in fixed point, as exp_c's does (section 3.2),
// with its values and their operations: e ln2 is |128 e| ln2/128 from ln2/128
// at 240 bits, truncated; |L_i| is held to 224 bits, as digits (below); r =
// rh + rl exactly, a multiple of 2^-106; log1p(r) by its series, summed
// until its terms vanish, each power and each quotient truncated; the
// positive parts and the negative ones apart, then taken from each other:
// within @B3U@ units of 2^-224 of log(x), on either side, which is less
// than @B3@ of it, |log(x)| being at least 2^-53 (proofs/log_phase3.py).
// That is far below the 2^-118.0 by which the hardest known argument's
// logarithm misses a rounding boundary, relatively, so the computed value
// rounds as the exact one does.

// D_INTERNAL_MATH_D_LN_DIGITS_A, D_INTERNAL_MATH_D_LN_DIGITS_B
//   the third phase's digits, in hexadecimal: for i from 0 to 128,
// |-log(inv_i)| times 2^224, rounded down, in 56 digits each; i from 0 to 64
// in the first literal, 3,640 digits, the rest in the second, each within
// C99's 4,095 characters.
#define D_INTERNAL_MATH_D_LN_DIGITS_A                                          \
@DIGITS_A@

#define D_INTERNAL_MATH_D_LN_DIGITS_B                                          \
@DIGITS_B@

// d_internal_math_d_ln_digits
//   |L_i| times 2^224 from its 56 digits: limbs from the last digit up, each
// of 4 digits.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_ln_digits(
    int _i
)
{
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    int k = 0;

    for (k = 0; k < 56; ++k)
    {
        const int  at    = (56 * _i) + k;
        const char c     = (at < 3640)
            ? D_INTERNAL_MATH_D_LN_DIGITS_A[at]
            : D_INTERNAL_MATH_D_LN_DIGITS_B[at - 3640];
        const int  place = 55 - k;
        const unsigned long digit = (c <= '9')
            ? (unsigned long)(c - '0')
            : (unsigned long)((c - 'a') + 10);

        result.limb[place / 4] |= digit << (4 * (place % 4));
    }

    return result;
}

// d_internal_math_d_ln_phase3
//   log(x) from _a in fixed point, correctly rounded: the positive parts in
// up and the negative ones in down - e ln2 of e's sign, L_i positive below
// 53 and negative from 53, and log1p(r)'s terms, r^k/k, each of the sign
// (-1)^(k+1) r^k has; up - down then rounded at 53 bits, as the exact
// value is: no rounding boundary is known to lie so close (the note above).
// rh and rl, multiples of 2^-106 below 2^-8, are each exact in fixed point.
D_MATH_FN double
d_internal_math_d_ln_phase3(
    struct dj_internal_math_d_ln_arg _a
)
{
    struct dj_internal_math_d_exp_fixed up   = { { 0 } };
    struct dj_internal_math_d_exp_fixed down = { { 0 } };
    const struct dj_internal_math_d_exp_fixed l =
        d_internal_math_d_ln_digits(_a.i);
    const struct dj_internal_math_d_exp_fixed h =
        d_internal_math_d_exp_fixed_of(_a.rh);
    const struct dj_internal_math_d_exp_fixed g =
        d_internal_math_d_exp_fixed_of(_a.rl);
    // |r|: |rl| below |rh| where rh is not 0, and 0 where it is
    const struct dj_internal_math_d_exp_fixed r =
        ((_a.rh < 0.0) == (_a.rl < 0.0))
            ? d_internal_math_d_exp_fixed_add(h, g)
            : d_internal_math_d_exp_fixed_sub(h, g);
    const bool negative = (_a.rh < 0.0);
    struct dj_internal_math_d_exp_fixed power = r;
    unsigned long k = 1;

    if (_a.e > 0)
    {
        up = d_internal_math_d_exp_ln2_times(128.0 * (double)_a.e);
    }
    else if (_a.e < 0)
    {
        down = d_internal_math_d_exp_ln2_times(128.0 * (double)_a.e);
    }

    if (_a.i < 53)
    {
        up = d_internal_math_d_exp_fixed_add(up, l);
    }
    else
    {
        down = d_internal_math_d_exp_fixed_add(down, l);
    }

    for (k = 1; k < 64; ++k)
    {
        const struct dj_internal_math_d_exp_fixed term =
            d_internal_math_d_exp_fixed_div(power, k);

        if (d_internal_math_d_exp_fixed_bits(term) == 0)
        {
            break;
        }

        if (negative || ((k % 2) == 0))
        {
            down = d_internal_math_d_exp_fixed_add(down, term);
        }
        else
        {
            up = d_internal_math_d_exp_fixed_add(up, term);
        }

        power = d_internal_math_d_exp_fixed_mul(power, r);
    }

    {
        const bool below = d_internal_math_d_exp_fixed_less(up, down);
        const struct dj_internal_math_d_exp_fixed d =
            below ? d_internal_math_d_exp_fixed_sub(down, up)
                  : d_internal_math_d_exp_fixed_sub(up, down);
        const int    place = d_internal_math_d_exp_fixed_bits(d) - 53;
        const double m     = d_internal_math_d_exp_fixed_round(d, place);
        // m 2^(place - 224), place - 224 between -107 and -43
        const double value =
            d_internal_math_d_exp_times_pow2(m, place - 224);

        return below ? -value : value;
    }
}

// d_internal_math_d_ln_slow
//   log(_x 2^_lift), _x positive, normal and finite, by the second phase
// and, where it does not decide, the third: the arguments the first leaves.
// Out of line, so that log_c's common path, inlined into its callers, keeps
// the small frame the first phase needs.
D_MATH_FN_COLD double
d_internal_math_d_ln_slow(
    double _x,
    int    _lift
)
{
    const struct dj_internal_math_d_ln_arg a =
        d_internal_math_d_ln_arg_of(_x, _lift);
    const struct dj_internal_math_d_exp_result two =
        d_internal_math_d_ln_phase2(a);

    return two.decided ? two.value : d_internal_math_d_ln_phase3(a);
}

// d_internal_math_d_ln_of
//   log(_x 2^_lift), _x positive, normal and finite: phase 1's result,
// where it decides the rounding; else the slow phases'.
D_MATH_FN double
d_internal_math_d_ln_of(
    double _x,
    int    _lift
)
{
    const struct dj_internal_math_d_exp_result one =
        d_internal_math_d_ln_phase1(d_internal_math_d_ln_arg_of(_x, _lift));

    return one.decided ? one.value : d_internal_math_d_ln_slow(_x, _lift);
}

// d_math_d_log_c
//   log(_x), correctly rounded; see above. A positive normal _x, every
// double but the rest, is taken in one test (S432); log(1) is +0 as the
// phases give it.
D_MATH_FN double
d_math_d_log_c(
    double _x
)
{
    // 2^54, by which a subnormal is lifted into the normal range
    const double lift = 18014398509481984.0;

    D_MATH_DBL_REFUSE(_x)

    if (d_internal_math_d_ln_normal(_x))
    {
        return d_internal_math_d_ln_of(_x, 0);
    }

    // NaN and +inf are their own logarithms
    if ( (_x != _x) ||
         (_x > DBL_MAX) )
    {
        return _x;
    }

    // zero, of either sign, has -inf
    if (_x == 0.0)
    {
        return -d_internal_math_d_infinity();
    }

    // a negative value has none
    if (_x < 0.0)
    {
        return d_internal_math_d_nan();
    }

    // a positive subnormal, lifted
    return d_internal_math_d_ln_of(_x * lift, -54);
}

// d_math_d_log
//   log(_x), correctly rounded: log_c; the C library's where double is
// evaluated wider than double; see above.
D_MATH_FN_RT double
d_math_d_log(
    double _x
)
{
#if !D_MATH_DBL_EVAL_WIDE
    return d_math_d_log_c(_x);
#elif defined(__cplusplus)
    return std::log(_x);
#else
    return log(_x);
#endif  // !D_MATH_DBL_EVAL_WIDE
}
"""


def log_d_section():
    """the double family's section 3.3, its constants filled in."""
    consts, rows, digits = log_d_constants()
    text = LOG_D_SECTION.replace("@TABLE@", log_d_table_macro(rows))
    for key in ("LN2H", "LN2M", "LN2L", "LN2L1"):
        text = text.replace("@" + key + "@", consts[key])
    for i, h in enumerate(LOG_D_Q):
        text = text.replace("@Q%d@" % i, exp_d_hex_literal(h))
    for i, (h, l) in enumerate(LOG_D_P):
        if l is None:
            text = text.replace("@P%d@" % i, exp_d_hex_literal(h))
        else:
            text = text.replace("@P%dH@" % i, exp_d_hex_literal(h))
            text = text.replace("@P%dL@" % i, exp_d_hex_literal(l))
    third = LOG_D_SECTION_3
    for key, part in (("@DIGITS_A@", digits[:3640]),
                      ("@DIGITS_B@", digits[3640:])):
        lines = exp_d_digits_literal("", part, 56)
        out = []
        for i, line in enumerate(lines):
            if i + 1 < len(lines):
                out.append(line + " " * (79 - len(line)) + "\\")
            else:
                out.append(line)
        third = third.replace(key, "\n".join(out))
    text += third
    for key, value in (("@B1A@", LOG_D_BOUNDS["p1a"]),
                       ("@B1@", LOG_D_BOUNDS["p1"]),
                       ("@QERR@", LOG_D_BOUNDS["qerr"]),
                       ("@KA@", str(LOG_D_BOUNDS["ka"])),
                       ("@KAPPA_TEXT@", LOG_D_BOUNDS["kappa_text"]),
                       ("@KAPPA@", "(%d.0 / %d.0)" % LOG_D_BOUNDS["kappa"]),
                       ("@EA@", "(1.0 / %d.0)" % (1 << LOG_D_BOUNDS["ka"])),
                       ("@B2@", LOG_D_BOUNDS["p2"]),
                       ("@B3U@", LOG_D_BOUNDS["p3u"]),
                       ("@B3@", LOG_D_BOUNDS["p3"])):
        text = text.replace(key, value)
    assert "@" not in text.replace("@param", "")
    return text


def log_d_swap(text):
    """the double family's text with its natural logarithm replaced."""
    a = text.index("// 3.3    Natural logarithm\n")
    b = text.index("// 3.4    Base-2 exponential\n")
    return text[:a] + log_d_section() + "\n" + text[b:]
